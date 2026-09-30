const assert = require('assert');
const fs = require('fs');
const http = require('http');
const path = require('path');
const {spawn} = require('child_process');

const root = path.resolve(__dirname, '..');
const source = path.join(root, 'docs', 'images', 'theme', 'hmi-web-demo.html');
const chromePath = 'C:/Program Files/Google/Chrome/Application/chrome.exe';
const debugPort = 9241;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

const server = http.createServer((request, response) => {
  const requested = new URL(request.url, 'http://localhost').pathname;
  if (request.method !== 'GET' || requested !== '/hmi-web-demo.html') {
    response.writeHead(404);
    response.end();
    return;
  }
  response.setHeader('content-type', 'text/html; charset=utf-8');
  response.end(fs.readFileSync(source));
});

(async () => {
  let chrome;
  let ws;
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const httpPort = server.address().port;
  const profile = path.join(root, 'build', 'p1-browser-profile-durable');
  chrome = spawn(chromePath, [
    '--headless=new',
    '--no-sandbox',
    '--disable-gpu',
    '--no-first-run',
    '--no-default-browser-check',
    '--remote-debugging-address=127.0.0.1',
    '--remote-debugging-port=' + debugPort,
    '--user-data-dir=' + profile,
    'about:blank'
  ], {windowsHide: true, stdio: 'ignore'});

  try {
    let pages;
    for (let attempt = 0; attempt < 60; attempt += 1) {
      try {
        pages = await (await fetch('http://127.0.0.1:' + debugPort + '/json')).json();
        break;
      } catch {
        await sleep(200);
      }
    }
    assert(pages, 'Chrome debugging endpoint unavailable');
    const page = pages.find(item => item.type === 'page');
    assert(page, 'Chrome page unavailable');

    ws = new WebSocket(page.webSocketDebuggerUrl);
    await new Promise((resolve, reject) => {
      ws.onopen = resolve;
      ws.onerror = reject;
    });
    let commandId = 0;
    const pending = new Map();
    const exceptions = [];
    ws.onmessage = event => {
      const message = JSON.parse(event.data);
      if (message.method === 'Runtime.exceptionThrown') {
        exceptions.push(message.params.exceptionDetails);
      }
      if (message.id && pending.has(message.id)) {
        const request = pending.get(message.id);
        pending.delete(message.id);
        if (message.error) request.reject(message.error);
        else request.resolve(message.result);
      }
    };
    const call = (method, params = {}) => new Promise((resolve, reject) => {
      const id = ++commandId;
      pending.set(id, {resolve, reject});
      ws.send(JSON.stringify({id, method, params}));
    });
    const evaluate = async expression => {
      const result = await call('Runtime.evaluate', {
        expression,
        returnByValue: true,
        awaitPromise: true
      });
      assert(!result.exceptionDetails, JSON.stringify(result.exceptionDetails));
      return result.result && result.result.value;
    };
    const waitFor = async expression => {
      for (let attempt = 0; attempt < 50; attempt += 1) {
        if (await evaluate(expression)) return;
        await sleep(100);
      }
      assert.fail('Timed out waiting for ' + expression);
    };

    await call('Runtime.enable');
    await call('Page.enable');
    await call('Emulation.setDeviceMetricsOverride', {
      width: 1024, height: 720, deviceScaleFactor: 1, mobile: false
    });
    await call('Page.navigate', {
      url: 'http://127.0.0.1:' + httpPort + '/hmi-web-demo.html'
    });
    await waitFor('document.readyState === \'complete\' && !!document.querySelector(\'.demo-shell\')');
    assert.strictEqual(await evaluate('document.title'), 'Furnace HMI Web Demo');
    assert.strictEqual(await evaluate('document.querySelectorAll(\'.expo-toolbar button,.expo-toolbar select\').length'), 8);
    assert.strictEqual(await evaluate('document.getElementById(\'advanced-controls\').open'), false);
    await evaluate('document.getElementById(\'scenario-toggle\').click()');
    assert.strictEqual(await evaluate('document.getElementById(\'advanced-controls\').open'), true);
    assert.strictEqual(await evaluate('document.querySelectorAll(\'.scenario-group\').length'), 3);
    assert.strictEqual(await evaluate('[...document.querySelectorAll(\'.scenario-buttons button\')].map(button => button.textContent).join(\'|\')'), 'Show letter keyboard|Show numeric keypad|Show running example|Advance run 1 min');
    assert.strictEqual(await evaluate('(()=>{const p=programCatalogue;return p.length===12&&Math.max(...p.map(x=>x.maximum))<=200&&Math.max(...p.flatMap(x=>x.stages.map(s=>s.target)))<=200&&Math.max(...p.flatMap(x=>x.stages.map(s=>Math.abs(s.rateTenths))))<=30})()'), true);
    await evaluate('document.getElementById(\'demo-speed\').value=\'300\';document.getElementById(\'demo-speed\').dispatchEvent(new Event(\'change\'))');
    assert.strictEqual(await evaluate('document.getElementById(\'demo-speed\').value'), '300');
    await evaluate('document.getElementById(\'try-text-keyboard\').click()');
    assert.strictEqual(await evaluate('!document.getElementById(\'touch-keyboard\').hidden'), true);
    assert.strictEqual(await evaluate('document.querySelectorAll(\'#touch-keyboard .keyboard-row\')[0].textContent'), '1234567890');
    assert.strictEqual(await evaluate('document.querySelectorAll(\'#touch-keyboard .keyboard-row\').length'), 4);
    assert.strictEqual(await evaluate('[...document.querySelectorAll(\'#touch-keyboard button\')].some(key => key.textContent === \'#+=\' || key.textContent === \'Symbols\')'), false);
    await evaluate('[...document.querySelectorAll(\'#touch-keyboard button\')].find(key => key.textContent === \'1\').click()');
    await evaluate('[...document.querySelectorAll(\'#touch-keyboard button\')].find(key => key.textContent === \'q\').click()');
    await evaluate('[...document.querySelectorAll(\'#touch-keyboard .keyboard-row\')[3].querySelectorAll(\'button\')][0].click()');
    await evaluate('[...document.querySelectorAll(\'#touch-keyboard button\')].find(key => key.textContent === \'Q\').click()');
    await sleep(1200);
    assert.strictEqual(await evaluate('document.getElementById(\'keyboard-input\').value'), '1qQ');
    assert.strictEqual(await evaluate('!document.getElementById(\'touch-keyboard\').hidden'), true);
    await evaluate('closeTouchKeyboard()');
    await evaluate('document.getElementById(\'demo-example\').click()');
    await sleep(300);
    assert.strictEqual(await evaluate('!document.getElementById(\'next-stage\').disabled'), true);
    const beforeAutomatic = await evaluate('demoRun.elapsed');
    await sleep(1100);
    assert.strictEqual(await evaluate('demoRun.elapsed > ' + beforeAutomatic), true);
    assert.strictEqual(await evaluate('(()=>{const line=document.querySelector(\'.run-line\'),items=[...line.querySelectorAll(\'.run-reading\')],tops=new Set(items.map(i=>Math.round(i.getBoundingClientRect().top)));return items.map(i=>i.querySelector(\'small\').textContent).join(\'|\')===\'Elapsed|Remaining|Current|Target|Est. power\'&&tops.size===1&&line.scrollWidth<=line.clientWidth+1})()'), true);
    const beforeManual = await evaluate('demoRun.elapsed');
    await evaluate('document.getElementById(\'demo-advance\').click()');
    assert.strictEqual(await evaluate('demoRun.elapsed >= ' + (beforeManual + 1)), true);
    await evaluate('document.getElementById(\'freeze-toggle\').click()');
    assert.strictEqual(await evaluate('document.getElementById(\'freeze-toggle\').getAttribute(\'aria-pressed\')'), 'true');
    await evaluate('document.getElementById(\'freeze-toggle\').click()');

    // Automatic cooling keeps one line: curing Elapsed plus cooling time and its estimate only.
    await evaluate('demoRun.phase=\'cooling\';demoRun.coolingElapsed=17;demoRun.coolingRemaining=25;demoRun.coolingTemperature=102;render()');
    assert.strictEqual(await evaluate('(()=>{const line=document.querySelector(\'.run-line\'),labels=[...line.querySelectorAll(\'.run-reading small\')].map(x=>x.textContent);return labels.join(\'|\')===\'Elapsed|Cooling|Est. remaining|Current|Target\'&&!document.querySelector(\'#content .cooling-summary\')&&line.scrollWidth<=line.clientWidth+1})()'), true);
    await evaluate('stopDemo()');
    await evaluate('switchMain(\'manual\')');
    assert.strictEqual(await evaluate('mainPage'), 'manual');
    assert.strictEqual(await evaluate('document.querySelector(\'.header h1\').textContent'), 'Manual mode');
    assert.strictEqual(await evaluate('!!document.getElementById(\'top-light\')'), true);
    const lightBounds = await evaluate('(()=>{const light=document.getElementById(\'top-light\'),target=light.getBoundingClientRect(),screen=document.querySelector(\'.screen\').getBoundingClientRect();return {height:target.height,width:target.width,screen:screen.width,tag:light.tagName}})()');
    assert(lightBounds.height >= 12 && lightBounds.width >= lightBounds.screen - 2 && lightBounds.tag === 'BUTTON', JSON.stringify(lightBounds));
    await evaluate('document.getElementById(\'top-light\').click()');
    await sleep(300);
    assert.strictEqual(await evaluate('controller.light && document.getElementById(\'top-light\').classList.contains(\'light-on\')'), true);
    const manualLayout = await evaluate('(()=>{const graph=document.querySelector(\'.manual-graph\'),card=document.querySelector(\'.manual-graph .program-graph\'),controls=document.querySelector(\'.manual-controls\');return {display:getComputedStyle(graph).display,card:card.getBoundingClientRect().height,graph:graph.getBoundingClientRect().height,controls:controls.getBoundingClientRect().height}})()');
    assert(manualLayout.display === 'flex' && manualLayout.card > manualLayout.graph * .7 && manualLayout.graph >= manualLayout.controls * 2, JSON.stringify(manualLayout));
    // The Manual graph must be drawn at its own layout size (1:1), never stretched to fit.
    const manualScale = await evaluate('(()=>{const svg=document.querySelector(\'.manual-graph svg\'),box=svg.viewBox.baseVal;return {viewWidth:box.width,viewHeight:box.height,width:svg.clientWidth,height:svg.clientHeight,aspect:svg.getAttribute(\'preserveAspectRatio\')}})()');
    assert(manualScale.aspect === null && manualScale.viewWidth > 0 && Math.abs(manualScale.viewWidth - manualScale.width) <= 1 && Math.abs(manualScale.viewHeight - manualScale.height) <= 1, JSON.stringify(manualScale));
    assert.strictEqual(await evaluate('(()=>{const controls=document.querySelector(\'.manual-controls\'),context=document.querySelector(\'.manual-readings\'),row=document.querySelector(\'.manual-control\'),controlRow=document.querySelector(\'.manual-control-row\'),stack=document.querySelector(\'.manual-control-stack\'),label=row.querySelector(\'span\'),decrease=row.querySelector(\'[aria-label^="Decrease"]\'),actions=document.querySelector(\'.manual-actions\');const labelRect=label.getBoundingClientRect(),decreaseRect=decrease.getBoundingClientRect(),controlsRect=controls.getBoundingClientRect(),contextRect=context.getBoundingClientRect(),controlRowRect=controlRow.getBoundingClientRect(),stackRect=stack.getBoundingClientRect(),actionsRect=actions.getBoundingClientRect();return !controls.contains(context)&&contextRect.bottom<=controlRowRect.top&&decreaseRect.width>=48&&decreaseRect.height>=56&&actionsRect.left<stackRect.left&&controlsRect.bottom-actionsRect.bottom<=13})()'), true);
    await evaluate('document.querySelector(\'[aria-label="Increase Target temperature"]\').click()');
    assert.strictEqual(await evaluate('manualDraft.target'), 121);
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'Start\').click()');
    await sleep(300);
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'121 C\').click()');
    await sleep(1200);
    assert.strictEqual(await evaluate('!document.getElementById(\'touch-keyboard\').hidden'), true);
    // Layout pixels (not on-screen pixels): the demo scales the HMI to fit the window.
    const manualGraphBounds = await evaluate('(()=>{const graph=document.querySelector(\'.manual-graph\'),card=graph.querySelector(\'.program-graph\'),svg=card.querySelector(\'svg\'),rect=o=>({width:o.offsetWidth??o.clientWidth,height:o.offsetHeight??o.clientHeight});return {graph:rect(graph),card:rect(card),svg:rect(svg)}})()');
    assert.strictEqual(manualGraphBounds.card.width >= manualGraphBounds.graph.width - 1 && manualGraphBounds.svg.width >= manualGraphBounds.card.width - 18 && manualGraphBounds.svg.height >= manualGraphBounds.card.height - 40, true);
    assert.strictEqual(await evaluate('(()=>{const labels=[...document.querySelectorAll(\'.manual-graph svg text\')].map(label=>label.textContent);return labels.includes(\'180 C\')&&labels.includes(\'0 C\')})()'), true);
    await evaluate('closeTouchKeyboard(false)');
    assert.strictEqual(await evaluate('demoRun.mode'), 'manual');
    assert.strictEqual(await evaluate('mainPage'), 'manual-running');
    await evaluate('document.querySelector(\'[aria-label="Decrease Rate"]\').click()');
    await sleep(300);
    assert.strictEqual(await evaluate('demoRun.rate'), 1.9);
    await evaluate('advanceDemo(controllerLimits().maxCuringMinutes)');
    assert.strictEqual(await evaluate('demoRun'), null);
    assert.strictEqual(await evaluate('mainPage'), 'manual');
    assert.match(await evaluate('document.getElementById(\'content\').textContent'), /cooling is automatic/i);

    await evaluate('switchMain(\'programs\')');
    await evaluate('programRoute=\'detail\';selectedProgram=0;render()');
    assert.strictEqual(await evaluate('[...document.querySelectorAll(\'.program-fact-row .program-fact span\')].map(x=>x.textContent).join(\'|\')'), 'Stages|Max. temp');
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'View stages\').click()');
    assert.strictEqual(await evaluate('document.querySelector(\'.stage-card .stage-meta\')'), null);
    assert.match(await evaluate('document.querySelector(\'.stage-card\').textContent'), /1[\\s\\S]*Heating[\\s\\S]*34 C[\\s\\S]*3 min/);
    await evaluate('programRoute=\'library\';render()');
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'New program\').click()');
    assert.strictEqual(await evaluate('mainPage'), 'program-editor');
    // The editor shows no statistics or explanatory notes and hides the bottom navigation.
    assert.strictEqual(await evaluate('!document.querySelector(\'#content .cooling-summary,#content .cooling-note\')&&getComputedStyle(document.querySelector(\'.nav\')).display===\'none\''), true);
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'+ Add stage\').click()');
    assert.strictEqual(await evaluate('mainPage'), 'stage-edit');
    await evaluate('saveStageEditor()');
    assert.strictEqual(await evaluate('mainPage+\'|\'+document.querySelectorAll(\'.stage-grid button.stage-card\').length'), 'program-editor|1');
    // Leaving with unsaved changes asks first; Discard returns to the library.
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'Back\').click()');
    assert.strictEqual(await evaluate('document.getElementById(\'message\').open&&mainPage===\'program-editor\''), true);
    await evaluate('[...document.querySelectorAll(\'#message-actions button\')].find(button => button.textContent === \'Discard changes\').click()');
    assert.strictEqual(await evaluate('mainPage+\'|\'+(programEditor===null)'), 'programs|true');

    await evaluate('mainPage=\'device\';deviceTab=\'log\';render()');
    assert.strictEqual(await evaluate('!!document.querySelector(\'.device-log-list\')'), true);

    for (const [width, height] of [[800, 480], [800, 640], [1024, 600]]) {
      await call('Emulation.setDeviceMetricsOverride', {
        width: width + 48, height: height + 200, deviceScaleFactor: 1, mobile: false
      });
      const index = height === 480 ? 0 : height === 640 ? 1 : 2;
      await evaluate('document.getElementById(\'size\').selectedIndex=' + index + ';document.getElementById(\'size\').dispatchEvent(new Event(\'change\'))');
      assert.deepStrictEqual(await evaluate('(()=>{const s=document.querySelector(\'.screen\');return [s.offsetWidth,s.offsetHeight]})()'), [width, height]);
    }

    await evaluate('document.getElementById(\'viewport-mode\').click()');
    assert.deepStrictEqual(await evaluate('(()=>{const s=document.querySelector(\'.screen\'),r=s.getBoundingClientRect();return [s.offsetWidth,s.offsetHeight,Math.round(r.width),Math.round(r.height),document.getElementById(\'viewport-area\').classList.contains(\'inspect-mode\')]})()'), [1024, 600, 1024, 600, true]);
    await evaluate('document.getElementById(\'viewport-mode\').click()');
    await evaluate('document.getElementById(\'scenario-toggle\').click()');
    await evaluate('document.getElementById(\'size\').selectedIndex=0;document.getElementById(\'size\').dispatchEvent(new Event(\'change\'))');
    await evaluate('tell(\'P1 dialog\',\'Dialog fixture\')');
    assert.strictEqual(await evaluate('document.getElementById(\'message\').open'), true);
    assert.strictEqual(await evaluate('(()=>{const r=document.getElementById(\'message\').getBoundingClientRect();return r.left>=0&&r.top>=0&&r.right<=innerWidth+1&&r.bottom<=innerHeight+1})()'), true);
    await evaluate('document.getElementById(\'message\').close()');

    await call('Emulation.setDeviceMetricsOverride', {
      width: 390, height: 800, deviceScaleFactor: 1, mobile: true
    });
    await sleep(200);
    const phone = await evaluate('(()=>{const s=document.querySelector(\'.screen\'),r=s.getBoundingClientRect();return {scroll:document.documentElement.scrollWidth,inner:innerWidth,width:Math.round(r.width),hint:getComputedStyle(document.getElementById(\'rotate-hint\')).display}})()');
    assert(phone.scroll <= phone.inner + 1);
    assert(phone.width <= phone.inner);
    assert.notStrictEqual(phone.hint, 'none');
    await evaluate('document.getElementById(\'fullscreen-toggle\').click()');
    await sleep(300);
    const fullscreen = await evaluate('({actual:!!document.fullscreenElement,fallback:document.querySelector(\'.demo-shell\').classList.contains(\'is-fallback-fullscreen\')})');
    assert(fullscreen.actual || fullscreen.fallback);
    assert.strictEqual(exceptions.length, 0, JSON.stringify(exceptions));
    console.log('PASS: P1 HTML demo viewport, controls, keyboard, dialog, mobile fit, fullscreen fallback and exception checks.');
  } finally {
    if (ws) ws.close();
    if (chrome) chrome.kill();
    server.close();
  }
})().catch(error => {
  console.error(error.stack || error);
  process.exitCode = 1;
});
