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

// Theme switch check: a probe theme whose every token differs from every colour
// of the generated themes. On every listed screen, no rendered colour (text,
// background, border, outline, shadow, gradient, SVG fill/stroke, ::before and
// ::after) may still be a theme colour; one that is was written as a literal.
// Not HMI: the demo page's frame shadow around the screen.
const THEME_PROBE = `(()=>{
 const themeRgb=new Set(),probe={},rgbOf=v=>{const m=v.match(/^#([0-9a-f]{6})/i);return m?parseInt(m[1],16):null;};
 const declared=[...document.querySelectorAll('#hmi-theme-tokens')].map(s=>s.textContent).join('');
 for(const m of declared.matchAll(/--[a-z0-9-]+:(#[0-9a-f]{6}|rgba\\((\\d+),(\\d+),(\\d+),[^)]*\\))/gi)){
  themeRgb.add(m[2]!==undefined?(+m[2]<<16)|(+m[3]<<8)|+m[4]:rgbOf(m[1]));
 }
 let seed=0x2468ac;const used=new Set();
 for(const token of HMI_THEME_TOKENS){let rgb;do{seed=(Math.imul(seed,1103515245)+12345)>>>0;rgb=(seed>>>8)&0xffffff;}while(themeRgb.has(rgb)||used.has(rgb));used.add(rgb);
  const current=getComputedStyle(document.documentElement).getPropertyValue('--'+token).trim(),alpha=(current.match(/rgba\\([^,]+,[^,]+,[^,]+,([^)]+)\\)/)||[])[1];
  const r=rgb>>16,g=(rgb>>8)&255,b=rgb&255;probe[token]=alpha!==undefined?'rgba('+r+','+g+','+b+','+alpha+')':'#'+rgb.toString(16).padStart(6,'0');}
 const apply=on=>{for(const token of HMI_THEME_TOKENS){if(on)document.documentElement.style.setProperty('--'+token,probe[token]);else document.documentElement.style.removeProperty('--'+token);}themeColors=null;};
 const parse=value=>{const out=[];for(const m of value.matchAll(/rgba?\\(([\\d.]+),?\\s*([\\d.]+),?\\s*([\\d.]+)(?:,?\\s*\\/?\\s*([\\d.]+))?\\)/g)){if(m[4]!==undefined&&+m[4]===0)continue;out.push((Math.round(+m[1])<<16)|(Math.round(+m[2])<<8)|Math.round(+m[3]));}
  for(const m of value.matchAll(/color\\(srgb ([\\d.]+) ([\\d.]+) ([\\d.]+)(?: \\/ ([\\d.]+))?\\)/g)){if(m[4]!==undefined&&+m[4]===0)continue;out.push((Math.round(m[1]*255)<<16)|(Math.round(m[2]*255)<<8)|Math.round(m[3]*255));}return out;};
 const problems=[];let checked=0;
 const inspect=(el,where)=>{for(const pseudo of [null,'::before','::after']){const cs=getComputedStyle(el,pseudo);if(pseudo&&(cs.content==='none'||cs.content===''))continue;if(!pseudo&&(cs.display==='none'||cs.visibility==='hidden'))return;
   const props=['color','background-color','background-image','box-shadow','fill','stroke'];
   for(const side of ['top','right','bottom','left'])if(parseFloat(cs['border-'+side+'-width'])>0)props.push('border-'+side+'-color');
   if(cs.outlineStyle!=='none'&&parseFloat(cs.outlineWidth)>0)props.push('outline-color');
   // SVG elements that paint no fill report the default black; the demo frame shadows the screen root.
   if(el instanceof SVGElement&&(cs.fill==='none'||!['rect','polygon','path','circle','ellipse','text'].includes(el.tagName)))props.splice(props.indexOf('fill'),1);
   if(el.classList.contains('screen'))props.splice(props.indexOf('box-shadow'),1);
   if(el instanceof SVGElement&&cs.stroke==='none')props.splice(props.indexOf('stroke'),1);
   if(!(el instanceof SVGElement)){props.splice(props.indexOf('fill'),1);props.splice(props.indexOf('stroke'),1);}
   for(const prop of props)for(const rgb of parse(cs.getPropertyValue(prop))){checked++;if(themeRgb.has(rgb)&&problems.length<12)problems.push(where+' '+(el.id?'#'+el.id:el.tagName.toLowerCase()+'.'+[...el.classList].join('.'))+(pseudo||'')+' '+prop+' #'+rgb.toString(16).padStart(6,'0')+' "'+(el.textContent||'').trim().slice(0,24)+'"');}}};
 const walk=where=>{const roots=[document.querySelector('.screen')];if(document.getElementById('message').open)roots.push(document.getElementById('message'));for(const root of roots)for(const el of [root,...root.querySelectorAll('*')])inspect(el,where);};
 const states={home:'render()',notifications:"render();document.getElementById('open-notifications').click()",programs:"switchMain('programs')",detail:"switchMain('programs');openProgram(0)",
  editor:"switchMain('programs');openProgram(0);openProgramEditor(0)",stage:"switchMain('programs');openProgram(0);openProgramEditor(0);openStageEditor(1)",
  loading:'loadDemoProgram(0)',graph:"beginDemoRun(0,60);runView='graph';render()",split:"beginDemoRun(0,60);runView='split';render()",furnace:"beginDemoRun(0,60);runView='furnace';render()",
  fault:"playScenario('overtemperature');window.clearInterval(scenarioPlayer.timer);for(let i=0;i<70;i++)stepScenario();render()",
  manual:"mainPage='manual';render()",overview:"switchMain('device','overview')",log:"switchMain('device','log')",maintenance:"switchMain('device','maintenance')",history:"switchMain('device','history')",info:"switchMain('device','info')",
  display:"switchMain('settings');tab='display';render()",time:"switchMain('settings');tab='time';render();openClockDialog()",system:"switchMain('settings');tab='system';render();resetScopes()",
  connections:"demoFixtures.connection='enabled';switchMain('settings');tab='connections';render()",text:'render();previewTextKeyboard()',numeric:'render();previewNumericKeyboard()'};
 apply(true);
 try{for(const [name,js] of Object.entries(states)){resetDemoState();demoFixtures.connection='disabled';new Function(js)();walk(name);}}
 finally{resetDemoState();demoFixtures.connection='disabled';apply(false);render();}
 return JSON.stringify({checked,problems});
})()`;

// Pseudo-language check: every catalogue text becomes "[text~~~]" (about 30 %
// longer; printf conversions and {fields} kept). On every listed screen, visible
// text, aria-label and title with letters but no "[" did not come from the
// catalogue, unless it is data (program and device catalogue entries) or a
// language name, which is always shown in its own language.
const PSEUDO_PROBE = `(()=>{
 const pseudo={};for(const [id,text] of Object.entries(HMI_STRINGS.en)){if(!/[A-Za-z]/.test(text)||id==='UI_LAB_DECIMAL_SEPARATOR')continue;pseudo[id]='['+text+'~'.repeat(Math.ceil(text.length*.3))+']';}
 const data=new Set(['DEMO-1',...HMI_LANGUAGES.map(language=>language.name)]);
 for(const program of programCatalogue){data.add(program.name);if(program.material)data.add(program.material);}
 for(const part of deviceCatalogue){for(const value of Object.values(part))if(typeof value==='string')data.add(value);for(const task of part.tasks)for(const value of Object.values(task))if(typeof value==='string')data.add(value);}
 const isData=text=>[...data].some(value=>value&&text.includes(value));
 const found=[];
 const check=(text,where,what)=>{const value=(text||'').trim();if(value.length>1&&/[A-Za-z]/.test(value)&&!value.includes('[')&&!isData(value)&&found.length<12)found.push(where+' '+what+' "'+value.slice(0,40)+'"');};
 const walk=where=>{const roots=[document.querySelector('.screen')];if(document.getElementById('message').open)roots.push(document.getElementById('message'));
  for(const root of roots){const walker=document.createTreeWalker(root,NodeFilter.SHOW_TEXT);for(let n=walker.nextNode();n;n=walker.nextNode()){const el=n.parentElement;if(el&&el.closest('[hidden]'))continue;if(el&&getComputedStyle(el).display==='none')continue;check(n.nodeValue,where,'text');}
   for(const el of [root,...root.querySelectorAll('[aria-label],[title]')]){if(el.hasAttribute('aria-label'))check(el.getAttribute('aria-label'),where,'aria-label');if(el.hasAttribute('title'))check(el.getAttribute('title'),where,'title');}}};
 const states={home:'render()',notifications:"render();document.getElementById('open-notifications').click()",programs:"switchMain('programs')",detail:"switchMain('programs');openProgram(0)",
  editor:"switchMain('programs');openProgram(0);openProgramEditor(0)",stage:"switchMain('programs');openProgram(0);openProgramEditor(0);openStageEditor(1)",start:"switchMain('programs');openProgram(0);confirmStart(0)",
  loading:'loadDemoProgram(0)',graph:"beginDemoRun(0,60);runView='graph';render()",split:"beginDemoRun(0,60);runView='split';render()",furnace:"beginDemoRun(0,60);runView='furnace';render()",
  fault:"playScenario('overtemperature');window.clearInterval(scenarioPlayer.timer);for(let i=0;i<70;i++)stepScenario();render()",
  link:"playScenario('link-lost');window.clearInterval(scenarioPlayer.timer);for(let i=0;i<60;i++)stepScenario();render()",
  manual:"mainPage='manual';render()",overview:"switchMain('device','overview')",log:"switchMain('device','log')",maintenance:"switchMain('device','maintenance')",history:"switchMain('device','history')",info:"switchMain('device','info')",
  display:"switchMain('settings');tab='display';render()",dim:"switchMain('settings');tab='display';render();[...document.querySelectorAll('#content button.card')][0].click()",time:"switchMain('settings');tab='time';render();openClockDialog()",
  system:"switchMain('settings');tab='system';render();resetScopes()",connections:"demoFixtures.connection='enabled';switchMain('settings');tab='connections';render()",
  text:'render();previewTextKeyboard()',numeric:'render();previewNumericKeyboard()'};
 hmiStringOverride=pseudo;
 try{for(const [name,js] of Object.entries(states)){resetDemoState();demoFixtures.connection='disabled';new Function(js)();walk(name);}}
 finally{hmiStringOverride=null;resetDemoState();demoFixtures.connection='disabled';render();}
 return JSON.stringify(found);
})()`;

// Theme and Language cards: information while there is one option; with a second
// theme and language (injected here) they become choices that preview at once,
// and Discard restores the saved ones.
const PRESENTATION_CHOICE = `(()=>{
 const cards=()=>[...document.querySelectorAll('#content .card')],find=label=>cards().find(c=>c.querySelector('.card-label').textContent.includes(label));
 resetDemoState();switchMain('settings');tab='display';render();
 const single=[find('Language').tagName,find('Theme').tagName].join(',');
 HMI_THEMES.push({id:'probe',name:'UI_LAB_SETTINGS_DARK'});HMI_LANGUAGES.push({tag:'xx',name:'Pseudo'});HMI_STRINGS.xx={UI_LAB_SETTINGS_LANGUAGE:'Sprache'};
 try{render();const multiple=[find('Language').tagName,find('Theme').tagName].join(',');
  find('Language').click();[...document.querySelectorAll('#message-body .choice-grid button')].find(b=>b.textContent==='Pseudo').click();
  const language=document.querySelector('.screen').lang+':'+!!find('Sprache');
  find('Theme').click();[...document.querySelectorAll('#message-body .choice-grid button')].find(b=>!b.classList.contains('chosen')).click();
  const theme=document.documentElement.dataset.hmiTheme;
  [...document.querySelectorAll('#content button')].find(b=>b.textContent===t('UI_LAB_SETTINGS_DISCARD')).click();
  return [single,multiple,language,theme,document.querySelector('.screen').lang,document.documentElement.dataset.hmiTheme].join('|');}
 finally{HMI_THEMES.pop();HMI_LANGUAGES.pop();delete HMI_STRINGS.xx;resetDemoState();render();}
})()`;

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
    const waitFor = async (expression, attempts = 50) => {
      for (let attempt = 0; attempt < attempts; attempt += 1) {
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
    // Demo page: the website logo in its own colours is the tab icon and the bar mark; an intro
    // (logo, company, product, status) covers the page until it ends or is skipped; a development
    // watermark stays over the page without taking input.
    assert.strictEqual(await evaluate('(()=>{const l=document.querySelector("link[rel=icon]");const h=l?decodeURIComponent(l.href):"";return h.startsWith("data:image/svg+xml")&&h.includes("#34393c")&&h.includes("#566e7e")})()'), true);
    assert.strictEqual(await evaluate('document.querySelector(".site-brand svg [fill=\'#34393c\']") !== null'), true);
    assert.strictEqual(await evaluate('[...document.querySelectorAll("#demo-intro .intro-mark > div")].map(e => e.textContent).join("|")'), 'Telamorph|HMI Demo V2|Work in progress');
    await evaluate('skipDemoIntro()');
    await waitFor('!document.getElementById("demo-intro")');
    // One development note, in the page footer below the HMI; the bar pill says Work in progress.
    assert.strictEqual(await evaluate('(()=>{const w=document.querySelectorAll(".dev-watermark");return w.length===1&&!!w[0].closest(".demo-foot")&&/Software in development/.test(w[0].textContent)&&document.querySelector(".live-pill").textContent==="Work in progress"})()'), true);
    // Demo page: website bar, one control panel with its sections, six scenarios (no Door opened).
    assert.deepStrictEqual(await evaluate('[...document.querySelectorAll(\'.site-actions a\')].map(a => a.href)'), ['https://telamorph.com/industrial.html', 'https://telamorph.com/contact.html?type=industrial']);
    assert.strictEqual(await evaluate('[...document.querySelectorAll(\'.panel-heading\')].map(h => h.textContent).join(\'|\')'), 'Screen|Playback|Scenarios|On-screen keyboards|Program library');
    assert.strictEqual(await evaluate('document.querySelectorAll(\'.scenario-item\').length'), 6);
    assert.strictEqual(await evaluate('!document.querySelector("[data-scenario=door-open]")'), true);
    assert.strictEqual(await evaluate('document.querySelector(\'#demo-speed [aria-pressed="true"]\').dataset.value'), '300');
    assert.strictEqual(await evaluate('document.getElementById(\'next-stage\').disabled && !document.getElementById(\'start-example\').hidden'), true);
    // A screen-size change draws the page again: Quick launch keeps only what fits the new size
    // (it used to keep the taller size's programs, clipped) and gets them back on the taller size.
    const quickCount = 'document.querySelectorAll(".quick-launch .quick-item").length';
    const quickFits = '(()=>{const q=document.querySelector(".quick-launch");return q.scrollHeight<=q.clientHeight+1})()';
    await evaluate('window.savedQuick=[[...programFavourites],[...demoRecent]];setDemoSize("800x640");programCatalogue.forEach(p=>programFavourites.add(p.id));demoRecent.splice(0,demoRecent.length,...programCatalogue.map(p=>p.id));mainPage="home";render()');
    const tallQuick = await evaluate(quickCount);
    await evaluate('setDemoSize("800x480")');
    assert.strictEqual(await evaluate(quickFits), true);
    assert.ok(await evaluate(quickCount) <= tallQuick);
    await evaluate('setDemoSize("800x640")');
    assert.strictEqual(await evaluate(quickCount), tallQuick);
    await evaluate('programFavourites.clear();savedQuick[0].forEach(id=>programFavourites.add(id));demoRecent.splice(0,demoRecent.length,...savedQuick[1]);setDemoSize("800x480")');
    // Program library: three epoxy programs, or none; programs the operator creates stay listed either way.
    assert.strictEqual(await evaluate('visiblePrograms().map(p => p.name).join("|")'), 'Epoxy system 1|Epoxy system 2|Epoxy system 3');
    assert.strictEqual(await evaluate('[...document.querySelectorAll("#demo-library button")].map(b => b.textContent).join("|")'), '3 programs|No programs');
    // A new program: one stage, a name, Save. Its details open; it is listed, starts and runs like a library
    // program, survives the library being switched off, and Delete removes it from the library and Recent programs.
    await evaluate('mainPage="programs";programRoute="library";render();openProgramEditor(-1);programEditor.name="Customer cure A";addProgramStage();saveStageEditor();saveProgramEditor()');
    await waitFor('!controller.pending && mainPage === "programs" && programRoute === "detail"');
    assert.strictEqual(await evaluate('(()=>{const p=programCatalogue[selectedProgram];return p.name==="Customer cure A"&&!p.library&&p.count===1&&p.stages[0].target===120&&document.querySelector(".program-toolbar h2").textContent==="Customer cure A"})()'), true);
    const created = await evaluate('selectedProgram');
    await evaluate('demoFixtures.program="empty";render()');
    assert.strictEqual(await evaluate('visiblePrograms().map(p => p.name).join("|")'), 'Customer cure A');
    await evaluate('demoFixtures.program="full";startDemo()');
    await waitFor('!!demoRun && mainPage === "running"');
    assert.strictEqual(await evaluate('demoRun.program.name + "|" + demoRecent[0]'), 'Customer cure A|' + created);
    // Stop during heating: the run is cut where it stands and cools automatically on Running, with
    // Start again (the same program from the start) and Return Home in place of Pause and Stop.
    await evaluate('advanceDemo(40);stopDemo()');
    assert.strictEqual(await evaluate('demoRun.stopped+"|"+demoRun.phase+"|"+demoRun.program.minutes+"|"+mainPage+"|"+[...document.querySelectorAll(".run-controls>button")].map(b=>b.textContent).join(",")+"|"+document.querySelector(".run-title span").textContent'), 'true|cooling|40|running|Start again,Return Home|Stopped · automatic cooling');
    assert.strictEqual(await evaluate('(()=>{const before=demoRun.coolingTemperature;advanceDemo(10);return demoRun.coolingTemperature<before&&demoRun.coolingElapsed===10})()'), true);
    await evaluate('[...document.querySelectorAll(".run-controls>button")].find(b=>b.textContent==="Start again").click()');
    await waitFor('!controller.pending && !!demoRun && !demoRun.stopped && demoRun.phase === "curing"');
    assert.strictEqual(await evaluate('demoRun.elapsed+"|"+demoRun.program.minutes+"|"+demoRun.program.name'), '0|95|Customer cure A');
    await evaluate('stopDemo();[...document.querySelectorAll(".run-controls>button")].find(b=>b.textContent==="Return Home").click()');
    assert.strictEqual(await evaluate('demoRun===null&&mainPage==="home"'), true);
    await evaluate('mainPage="programs";programRoute="detail";selectedProgram=' + created + ';render();deleteProgram(' + created + ')');
    await waitFor('!controller.pending && document.getElementById("message").open');
    assert.strictEqual(await evaluate('programCatalogue[' + created + '].deleted && !visiblePrograms().some(p => p.id === ' + created + ') && !demoRecent.includes(' + created + ') && document.getElementById("message-title").textContent'), 'Program deleted');
    await evaluate('document.getElementById("message").close();mainPage="home";render()');
    // Maintenance: a task card opens its details with a corner X (closes, nothing else) and Maintenance done,
    // which the controller records: the task starts a new interval, its last baseline is today, and the record
    // leads Service history and Last service.
    await evaluate('switchMain("device","maintenance");[...document.querySelectorAll("#content .card")].find(c=>c.textContent.includes("Inspect filter")).click()');
    assert.strictEqual(await evaluate('(()=>{const m=document.getElementById("message");return m.open&&!!m.querySelector(".dialog-close .icon")&&[...document.querySelectorAll("#message-actions button")].map(b=>b.textContent).join("|")})()'), 'Maintenance done');
    await evaluate('document.querySelector("#message .dialog-close").click()');
    await waitFor('!document.getElementById("message").open && !document.querySelector("#message .dialog-close")');
    await evaluate('[...document.querySelectorAll("#content .card")].find(c=>c.textContent.includes("Inspect filter")).click();[...document.querySelectorAll("#message-actions button")][0].click()');
    await waitFor('!controller.pending && document.getElementById("message").open');
    assert.strictEqual(await evaluate('document.getElementById("message-title").textContent'), 'Maintenance recorded');
    assert.strictEqual(await evaluate('(()=>{const task=deviceCatalogue.find(p=>p.id==="air_filter").tasks[0];return task.state+"|"+task.remaining+"|"+(task.last===shortDate(clockNow()))+"|"+serviceHistory[0].title})()'), 'ok|Next in 500 h|true|Air filter / Inspect filter');
    await evaluate('document.getElementById("message").close();switchMain("device","overview")');
    assert.ok(await evaluate('[...document.querySelectorAll("#content .card")].some(c=>c.textContent.includes("Air filter / Inspect filter"))'));
    // Device info: five cards (no Controller link); Support spans two columns and opens the QR code
    // (the generated mask in qr-ink on a qr-plate) with the corner X as its only way out.
    await evaluate('switchMain("device","info")');
    assert.strictEqual(await evaluate('[...document.querySelectorAll("#content .card .card-label")].map(e=>e.textContent).join("|")'), 'Model|HMI firmware|Controller firmware|Component catalogue|Support');
    await evaluate('[...document.querySelectorAll("#content .card")].find(c=>c.textContent.includes("telamorph.com")).click()');
    assert.strictEqual(await evaluate('(()=>{const code=document.querySelector("#message .support-qr-code"),style=code&&getComputedStyle(code);return !!style&&/data:image\\/png/.test(style.maskImage||style.webkitMaskImage)&&code.getBoundingClientRect().width>150&&!!document.querySelector("#message .dialog-close")&&!document.querySelector("#message-actions button")})()'), true);
    await evaluate('document.querySelector("#message .dialog-close").click()');
    await waitFor('!document.getElementById("message").open && !document.querySelector("#message .support-qr")');
    await evaluate('switchMain("home")');
    // Date format: a choice of five on Date & time; dates follow the edited format (the clock card and the log).
    await evaluate('switchMain("settings");tab="time";render()');
    const cardValue = label => '[...document.querySelectorAll("#content .card")].find(c=>c.querySelector(".card-label").textContent.includes("' + label + '")).querySelector(".card-value, strong").textContent';
    assert.strictEqual(await evaluate(cardValue('Date format')), await evaluate('dateText(clockNow(),"text")'));
    await evaluate('[...document.querySelectorAll("#content .card")].find(c=>c.textContent.includes("Date format")).click()');
    assert.strictEqual(await evaluate('[...document.querySelectorAll("#message-body .choice-grid button")].length'), 5);
    assert.strictEqual(await evaluate('[...document.querySelectorAll("#message-body .choice-grid button")].every(b=>b.scrollWidth<=b.clientWidth&&b.getClientRects().length===1&&b.offsetHeight<70)'), true);
    await evaluate('[...document.querySelectorAll("#message-body .choice-grid button")][4].click()');
    assert.match(await evaluate(cardValue('Date format')), /^\d{4}-\d{2}-\d{2}$/);
    assert.ok(await evaluate('clockDateText(clockNow()).endsWith(dateText(clockNow(),"iso"))'));
    await evaluate('saved={...candidate};render()');
    // Settings > System: Delete programs removes every program after one confirmation naming them (a single
    // program is deleted from its details); Reset (date & time formats) restores the default date format.
    await evaluate('tab="system";render();deletePrograms()');
    assert.strictEqual(await evaluate('document.getElementById("message-title").textContent+"|"+[...document.querySelectorAll("#message-actions button")].map(b=>b.textContent).join(",")'), 'Delete all 3 programs?|Cancel,Delete all');
    assert.match(await evaluate('document.getElementById("message-text").textContent'), /^Epoxy system 1, Epoxy system 2, Epoxy system 3\. /);
    await evaluate('[...document.querySelectorAll("#message-actions button")].find(b=>b.textContent==="Delete all").click()');
    await waitFor('!controller.pending && document.getElementById("message-title").textContent === "Programs deleted"');
    assert.strictEqual(await evaluate('visiblePrograms().length'), 0);
    await evaluate('document.getElementById("message").close();resetScopes();[...document.querySelectorAll("#message-actions button")].find(b=>b.textContent==="Date & time formats").click()');
    await evaluate('[...document.querySelectorAll("#message-actions button")].find(b=>b.textContent==="Reset preferences").click()');
    assert.strictEqual(await evaluate('saved.dateFormat+"|"+candidate.dateFormat'), 'text|text');
    await evaluate('document.getElementById("message").close();[0,1,2].forEach(id=>programCatalogue[id].deleted=false);switchMain("home")');
    // Below the wide layout the panel is a side menu opened from the bar; the page is one window tall.
    assert.strictEqual(await evaluate('(()=>{const s=document.getElementById(\'demo-shell\');return s.classList.contains(\'uses-drawer\')&&s.classList.contains(\'fits-view\')&&getComputedStyle(document.getElementById(\'bar-controls\')).display!==\'none\'&&document.documentElement.scrollHeight<=innerHeight})()'), true);
    await call('Emulation.setDeviceMetricsOverride', {width: 1600, height: 900, deviceScaleFactor: 1, mobile: false});
    await sleep(300);
    // Wide desktop: panel beside the floating screen (no frame ring), scenarios listed, no page scroll.
    const sectionOrder = '[...document.querySelectorAll(\'#control-panel > .panel-section\')].sort((a,b)=>a.getBoundingClientRect().top-b.getBoundingClientRect().top).map(e=>e.classList[1]).join(\'|\')';
    assert.strictEqual(await evaluate('(()=>{const s=document.getElementById(\'demo-shell\'),p=document.getElementById(\'control-panel\').getBoundingClientRect(),f=document.getElementById(\'viewport-frame\').getBoundingClientRect();return !s.classList.contains(\'uses-drawer\')&&p.left>=f.right&&getComputedStyle(document.querySelector(\'.scenario-list\')).display!==\'none\'&&!/0px 0px 0px 9px/.test(getComputedStyle(document.querySelector(\'.screen\')).boxShadow)&&document.documentElement.scrollHeight<=innerHeight})()'), true);
    assert.strictEqual(await evaluate(sectionOrder), 'panel-screen|panel-playback|panel-scenarios|panel-keyboards|panel-library');
    await call('Emulation.setDeviceMetricsOverride', {width: 390, height: 844, deviceScaleFactor: 1, mobile: true});
    await sleep(300);
    // Mobile side menu: the same order, scenarios listed (not folded).
    await evaluate('document.getElementById(\'bar-controls\').click()');
    await sleep(400);
    assert.strictEqual(await evaluate(sectionOrder), 'panel-screen|panel-playback|panel-scenarios|panel-keyboards|panel-library');
    assert.strictEqual(await evaluate('getComputedStyle(document.querySelector(\'.scenario-list\')).display!==\'none\'&&!document.querySelector(\'.panel-toggle\')'), true);
    await evaluate('document.getElementById(\'drawer-close\').click()');
    await call('Emulation.setDeviceMetricsOverride', {width: 1024, height: 720, deviceScaleFactor: 1, mobile: false});
    await sleep(300);
    assert.strictEqual(await evaluate('(()=>{const p=programCatalogue;return p.filter(x=>x.library).length===3&&Math.max(...p.map(x=>x.maximum))<=180&&Math.max(...p.flatMap(x=>x.stages.map(s=>s.target)))<=180&&Math.max(...p.flatMap(x=>x.stages.map(s=>Math.abs(s.rateTenths))))<=30})()'), true);
    await evaluate('document.querySelector(\'#demo-speed [data-value="60"]\').click()');
    assert.strictEqual(await evaluate('demoSpeed + \'|\' + document.querySelector(\'#demo-speed [aria-pressed="true"]\').dataset.value'), '60|60');
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
    await evaluate('document.getElementById(\'start-example\').click()');
    await sleep(300);
    assert.strictEqual(await evaluate('!document.getElementById(\'next-stage\').disabled && document.getElementById(\'start-example\').hidden'), true);
    const beforeAutomatic = await evaluate('demoRun.elapsed');
    await sleep(1100);
    assert.strictEqual(await evaluate('demoRun.elapsed > ' + beforeAutomatic), true);
    assert.strictEqual(await evaluate('(()=>{const line=document.querySelector(\'.run-line\'),items=[...line.querySelectorAll(\'.run-reading\')],tops=new Set(items.map(i=>Math.round(i.getBoundingClientRect().top)));return items.map(i=>i.querySelector(\'small\').textContent).join(\'|\')===\'Elapsed|Remaining|Current °C|Target °C|Est. power kW\'&&tops.size===1&&line.scrollWidth<=line.clientWidth+1})()'), true);
    // Playback: next/previous stage move between stage starts; Pause freezes the simulation clock.
    await evaluate('document.getElementById(\'next-stage\').click()');
    const stageAfterNext = await evaluate('runStage()');
    assert.strictEqual(await evaluate('demoRun.elapsed === stageStartsOf(demoRun.program)[runStage()]'), true);
    await evaluate('document.getElementById(\'stage-previous\').click()');
    assert.strictEqual(await evaluate('runStage()'), stageAfterNext - 1);
    await evaluate('document.getElementById(\'playback-toggle\').click()');
    assert.strictEqual(await evaluate('document.getElementById(\'playback-toggle\').getAttribute(\'aria-pressed\') + \'|\' + demoFrozen'), 'true|true');
    const frozenAt = await evaluate('demoRun.elapsed');
    await sleep(1100);
    assert.strictEqual(await evaluate('demoRun.elapsed'), frozenAt);
    await evaluate('document.getElementById(\'playback-toggle\').click()');

    // Automatic cooling keeps one line: curing Elapsed plus cooling time and its estimate only.
    await evaluate('demoRun.phase=\'cooling\';demoRun.coolingElapsed=17;demoRun.coolingRemaining=25;demoRun.coolingTemperature=102;render()');
    assert.strictEqual(await evaluate('(()=>{const line=document.querySelector(\'.run-line\'),labels=[...line.querySelectorAll(\'.run-reading small\')].map(x=>x.textContent);return labels.join(\'|\')===\'Elapsed|Cooling|Est. remaining|Current °C|Target °C\'&&!document.querySelector(\'#content .cooling-summary\')&&line.scrollWidth<=line.clientWidth+1})()'), true);
    // Stop during automatic cooling marks the run stopped; Return Home closes it.
    await evaluate('stopDemo()');
    assert.strictEqual(await evaluate('demoRun.stopped&&[...document.querySelectorAll(".run-controls>button")].map(b=>b.textContent).join(",")'), 'Start again,Return Home');
    await evaluate('leaveFaultedRun()');
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
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'121\').click()');
    await sleep(1200);
    assert.strictEqual(await evaluate('!document.getElementById(\'touch-keyboard\').hidden'), true);
    // Layout pixels (not on-screen pixels): the demo scales the HMI to fit the window.
    const manualGraphBounds = await evaluate('(()=>{const graph=document.querySelector(\'.manual-graph\'),card=graph.querySelector(\'.program-graph\'),svg=card.querySelector(\'svg\'),rect=o=>({width:o.offsetWidth??o.clientWidth,height:o.offsetHeight??o.clientHeight});return {graph:rect(graph),card:rect(card),svg:rect(svg)}})()');
    assert.strictEqual(manualGraphBounds.card.width >= manualGraphBounds.graph.width - 1 && manualGraphBounds.svg.width >= manualGraphBounds.card.width - 18 && manualGraphBounds.svg.height >= manualGraphBounds.card.height - 40, true);
    assert.strictEqual(await evaluate('(()=>{const labels=[...document.querySelectorAll(\'.manual-graph svg text\')].map(label=>label.textContent);return labels.includes(\'180 °C\')&&labels.includes(\'0 °C\')})()'), true);
    await evaluate('closeTouchKeyboard(false)');
    assert.strictEqual(await evaluate('demoRun.mode'), 'manual');
    assert.strictEqual(await evaluate('mainPage'), 'manual-running');
    await evaluate('document.querySelector(\'[aria-label="Decrease Rate"]\').click()');
    await sleep(300);
    assert.strictEqual(await evaluate('demoRun.rate'), 1.9);
    // At 48 h (as after Stop) the controller cools automatically: the session stays on screen with cooling
    // readings, the steppers off, and Exit / Start new; the temperature falls; Start new keeps target and rate.
    await evaluate('advanceDemo(controllerLimits().maxCuringMinutes)');
    assert.strictEqual(await evaluate('demoRun.phase + "|" + mainPage'), 'cooling|manual-running');
    assert.strictEqual(await evaluate('[...document.querySelectorAll(".manual-actions button")].map(b=>b.textContent).join("|")'), 'Exit|Start new');
    assert.strictEqual(await evaluate('(()=>{const s=document.querySelector(".manual-readings");return s.querySelector(".manual-state").textContent+"|"+[...s.querySelectorAll(".run-reading small")].map(e=>e.textContent).join(",")+"|"+[...document.querySelectorAll(".manual-control button")].every(b=>b.disabled)})()'), 'Cooling|Elapsed,Cooling,Est. remaining,Current °C,Target °C|true');
    assert.strictEqual(await evaluate('(()=>{const before=demoRun.temperature;advanceDemo(20);return demoRun.temperature<before&&demoRun.coolingElapsed===20&&switchMain("home")===undefined&&mainPage==="manual-running"})()'), true);
    await evaluate('[...document.querySelectorAll(".manual-actions button")].find(b=>b.textContent==="Start new").click()');
    assert.strictEqual(await evaluate('demoRun===null&&mainPage==="manual"&&manualDraft.target===121&&manualDraft.rate===1.9&&deviceLog.some(e=>e.type==="manual-stopped")'), true);

    await evaluate('switchMain(\'programs\')');
    await evaluate('programRoute=\'detail\';selectedProgram=0;render()');
    assert.strictEqual(await evaluate('[...document.querySelectorAll(\'.program-fact-row .program-fact span\')].map(x=>x.textContent).join(\'|\')'), 'Stages|Max. °C');
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'View stages\').click()');
    assert.strictEqual(await evaluate('document.querySelector(\'.stage-card .stage-meta\')'), null);
    assert.match(await evaluate('document.querySelector(\'.stage-card\').textContent'), /1[\\s\\S]*Heating[\\s\\S]*40 °C[\\s\\S]*50 min/i);
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
    // Device log: one month of controller events, newest first, filter tabs, date headings with their age,
    // pages that fit the screen; this session's events (display start, the run, maintenance) are on top.
    assert.strictEqual(await evaluate('[...document.querySelectorAll(".log-toolbar button")].map(b=>b.textContent).join("|")'), 'All|Programs|Alarms|Maintenance|System');
    assert.strictEqual(await evaluate('(()=>{const now=wallMinutes(clockNow()),oldest=deviceLog[deviceLog.length-1].at;return deviceLog.length>40&&now-oldest<30*1440&&deviceLog.every((e,i)=>i===0||e.at<=deviceLog[i-1].at)&&deviceLog.some(e=>e.type==="program-completed")&&deviceLog.some(e=>e.type==="alarm")})()'), true);
    assert.strictEqual(await evaluate('(()=>{const l=document.querySelector(".log-list"),first=l.firstElementChild;return first.classList.contains("log-day")&&/\\(today\\)/.test(first.textContent)&&l.scrollHeight<=l.clientHeight+1&&l.querySelectorAll(".log-row").length>=3})()'), true);
    assert.strictEqual(await evaluate('["display-started","program-saved","program-deleted","maintenance-done"].every(type=>deviceLog.some(e=>e.type===type&&e.at>=wallMinutes(clockNow())-60))'), true);
    assert.strictEqual(await evaluate('deviceLog.filter(e=>e.at>=wallMinutes(clockNow())-60).map(logText).filter(text=>/Customer cure A|Air filter/.test(text)).reverse().join("|")'), 'Program saved: Customer cure A|Program started: Customer cure A|Program stopped by the operator: Customer cure A|Program started: Customer cure A|Program stopped by the operator: Customer cure A|Program deleted: Customer cure A|Maintenance done: Air filter / Inspect filter');
    assert.strictEqual(await evaluate('document.querySelector(".log-row .log-text").textContent === logText(deviceLog[0])'), true);
    await evaluate('[...document.querySelectorAll(".log-toolbar button")].find(b=>b.textContent==="Alarms").click()');
    assert.strictEqual(await evaluate('[...document.querySelectorAll(".log-row .log-text")].every(e=>/^(Over-temperature|Target not reached|Door opened|Temperature sensor fault|Power interrupted): Epoxy system [123]$/.test(e.textContent))&&document.querySelectorAll(".log-row").length>0'), true);
    await evaluate('logFilter="all";devicePage=0;render();document.querySelector(".log-row").click()');
    assert.match(await evaluate('document.getElementById("message-text").textContent'), /^\d+ \w+ \d{4} at .+\. Recorded by: Control CPU\.$/);
    await evaluate('document.getElementById("message").close()');
    // Stale: paged card tabs mark it in the footer, so the cards and the pager still fit.
    assert.strictEqual(await evaluate('(()=>{demoFixtures.device=\'stale\';const ok=[\'maintenance\',\'history\'].every(t=>{deviceTab=t;render();const c=document.getElementById(\'content\');return c.scrollHeight<=c.clientHeight&&/Last known/.test(c.querySelector(\'.device-footer\').textContent)});deviceTab=\'info\';render();const info=!/Stale|Controller link/.test(document.getElementById(\'content\').textContent);deviceTab=\'overview\';render();const banner=/Last known snapshot/.test(document.getElementById(\'content\').textContent);demoFixtures.device=\'current\';deviceTab=\'log\';render();return ok&&info&&banner})()'), true);
    // Device cards: Overview six cards; Maintenance lists every task worst first, six per page; Info five cards (no Controller link).
    assert.strictEqual(await evaluate('(()=>{chooseDevice(\'overview\');const o=document.querySelectorAll(\'#content .card\').length;chooseDevice(\'maintenance\');const cards=[...document.querySelectorAll(\'#content button.card\')];const first=cards[0].classList.contains(\'card-due\');const count=document.querySelector(\'.device-footer span\').textContent;cards[0].click();const dialog=document.getElementById(\'message\').open;document.getElementById(\'message\').close();chooseDevice(\'info\');const i=document.querySelectorAll(\'#content button.card\').length;return [o,cards.length,first,count,dialog,i].join(\'|\')})()'), '6|6|true|8 items|true|5');
    // Settings cards: one Display page (no More); the brightness gauge (no number), dim choice (Off by default), Discard.
    assert.strictEqual(await evaluate('(()=>{switchMain(\'settings\');tab=\'display\';render();const more=[...document.querySelectorAll(\'#content button\')].some(b=>/More/.test(b.textContent))||/%/.test(document.querySelector(\'.gauge-card\').textContent)||candidate.dim!==0;document.querySelectorAll(\'.gauge-steps button\')[2].click();const b30=candidate.brightness;document.querySelector(\'.gauge button[aria-label="Increase brightness"]\').click();const b40=candidate.brightness;[...document.querySelectorAll(\'#content button.card\')].find(c=>/Dim when idle/.test(c.textContent)).click();const choices=[...document.querySelectorAll(\'#message-body .choice-grid button\')].map(b=>b.textContent).join(\',\');[...document.querySelectorAll(\'#message-body .choice-grid button\')].find(b=>b.textContent===\'5 min\').click();const dim=candidate.dim+\'|\'+/5 min/.test(document.getElementById(\'content\').textContent);[...document.querySelectorAll(\'#content button\')].find(b=>b.textContent===\'Discard\').click();return [more,b30,b40,choices,dim,candidate.brightness,candidate.dim].join(\'|\')})()'), 'false|30|40|Off,1 min,2 min,5 min,10 min,30 min|5|true|80|0');
    // Settings rail: Connections only when the controller enables it; System (power icon) holds reset/delete/restart; no Clock source.
    assert.strictEqual(await evaluate('(()=>{const rail=()=>[...document.querySelectorAll(\'.rail button\')].map(b=>b.textContent).join(\',\');const off=rail();const power=!!document.querySelector(\'.rail use[href="#i-power"]\');tab=\'time\';render();const clock=/Clock source/.test(document.getElementById(\'content\').textContent);demoFixtures.connection=\'enabled\';render();const on=rail();[...document.querySelectorAll(\'.rail button\')].find(b=>b.textContent===\'Connections\').click();const page=tab+\':\'+/Server/.test(document.getElementById(\'content\').textContent);demoFixtures.connection=\'disabled\';render();const fallback=tab;return [off,power,clock,on,page,fallback].join(\'|\')})()'), 'Display & interaction,Date & time,System|true|false|Display & interaction,Date & time,Connections,System|connections:true|display');
    // Set date & time: 31 Jan -> Feb clamps the day; Cancel changes nothing; Set moves the clock; 12-hour adds AM/PM.
    assert.strictEqual(await evaluate('(()=>{tab=\'time\';render();const before=clockOffsetMs;document.querySelector(\'.clock-card\').click();const cols=()=>[...document.querySelectorAll(\'#message-body .clock-column\')];const value=n=>cols()[n].querySelector(\'strong\').textContent;const step=(n,dir)=>cols()[n].querySelectorAll(\'button\')[dir>0?0:1].click();while(value(1)!==\'Jan\')step(1,1);while(value(0)!==\'31\')step(0,1);step(1,1);const clamped=(value(0)===String(daysInMonth(+value(2),2)))+\' \'+value(1);[...document.querySelectorAll(\'#message-actions button\')].find(b=>b.textContent===\'Cancel\').click();const unchanged=clockOffsetMs===before;document.querySelector(\'.clock-card\').click();step(2,1);[...document.querySelectorAll(\'#message-actions button\')].find(b=>b.textContent===\'Set date & time\').click();const moved=clockNow().getFullYear()===new Date().getFullYear()+1&&document.getElementById(\'message-title\').textContent===\'Date and time set\';document.getElementById(\'message\').close();clockOffsetMs=0;candidate.format=\'12-hour\';render();document.querySelector(\'.clock-card\').click();const twelve=cols().length===6&&/^(AM|PM)$/.test(value(5));document.getElementById(\'message\').close();candidate.format=\'24-hour\';render();return [clamped,unchanged,moved,twelve].join(\'|\')})()'), 'true Feb|true|true|true');
    await sleep(50);
    assert.strictEqual(await evaluate('document.getElementById(\'message-body\').children.length+\'|\'+dirty()'), '0|false');
    await evaluate('mainPage=\'device\';deviceTab=\'log\';render()');

    for (const [width, height] of [[800, 480], [800, 640], [1024, 600]]) {
      await call('Emulation.setDeviceMetricsOverride', {
        width: width + 48, height: height + 200, deviceScaleFactor: 1, mobile: false
      });
      await evaluate('document.querySelector(\'#demo-size [data-value="' + width + 'x' + height + '"]\').click()');
      assert.deepStrictEqual(await evaluate('(()=>{const s=document.querySelector(\'.screen\');return [s.offsetWidth,s.offsetHeight]})()'), [width, height]);
    }

    await evaluate('document.querySelector(\'#demo-scale [data-value="inspect"]\').click()');
    assert.deepStrictEqual(await evaluate('(()=>{const s=document.querySelector(\'.screen\'),r=s.getBoundingClientRect();return [s.offsetWidth,s.offsetHeight,Math.round(r.width),Math.round(r.height),document.getElementById(\'viewport-area\').classList.contains(\'inspect-mode\')]})()'), [1024, 600, 1024, 600, true]);
    await evaluate('document.querySelector(\'#demo-scale [data-value="fit"]\').click()');
    await evaluate('document.querySelector(\'#demo-size [data-value="800x480"]\').click()');
    await evaluate('tell(\'P1 dialog\',\'Dialog fixture\')');
    assert.strictEqual(await evaluate('document.getElementById(\'message\').open'), true);
    assert.strictEqual(await evaluate('(()=>{const r=document.getElementById(\'message\').getBoundingClientRect();return r.left>=0&&r.top>=0&&r.right<=innerWidth+1&&r.bottom<=innerHeight+1})()'), true);
    await evaluate('document.getElementById(\'message\').close()');

    // Scenarios: each fast-forwards to its event through the HMI's own fault and link paths.
    await evaluate('playScenario(\'overtemperature\')');
    await waitFor('demoRun && demoRun.phase === \'fault\'', 90);
    assert.strictEqual(await evaluate('(()=>{const d=document.getElementById(\'message\');return d.open&&d.classList.contains(\'alert-fault\')&&document.getElementById(\'message-title\').textContent})()'), 'Over-temperature');
    assert.strictEqual(await evaluate('(()=>{const b=[...document.querySelectorAll(\'#content button\')].map(x=>x.textContent);return b.includes(\'Return Home\')&&!b.includes(\'Pause\')&&!b.includes(\'Stop\')&&document.querySelector(\'.run-title .run-fault\').textContent===\'Over-temperature\'&&demoMeasured(demoRun.program,demoRun.elapsed)>180})()'), true);
    await evaluate('document.querySelector(\'#message-actions button\').click()');
    await sleep(100);
    assert.strictEqual(await evaluate('document.getElementById(\'message\').classList.contains(\'alert-dialog\')'), false);
    assert.match(await evaluate('notifications().map(n => n.title).join(\'|\')'), /Over-temperature/);
    await evaluate('[...document.querySelectorAll(\'#content button\')].find(button => button.textContent === \'Return Home\').click()');
    assert.strictEqual(await evaluate('mainPage + \'|\' + (demoRun === null)'), 'home|true');

    await evaluate('playScenario(\'link-lost\')');
    await waitFor('demoLinkLost', 90);
    assert.strictEqual(await evaluate('(()=>{const values=[...document.querySelectorAll(\'.run-line .run-reading strong\')].map(x=>x.textContent),stop=[...document.querySelectorAll(\'#content button\')].find(x=>x.textContent===\'Stop\');return values.every(v=>v===\'-- stale\')&&stop.disabled&&document.getElementById(\'message-title\').textContent===\'Connection lost\'})()'), true);
    await waitFor('!demoLinkLost && !scenarioPlayer', 60);
    assert.strictEqual(await evaluate('document.getElementById(\'message-title\').textContent'), 'Connection restored');
    await evaluate('document.getElementById(\'message\').close()');

    await evaluate('playScenario(\'power-loss\')');
    await waitFor('!scenarioPlayer && document.getElementById(\'message\').open', 120);
    assert.strictEqual(await evaluate('mainPage + \'|\' + document.getElementById(\'message-title\').textContent + \'|\' + demoFault'), 'home|Power interrupted|power-loss');
    await evaluate('resetDemoState();render();updateExpoControls()');

    // Theme switch: setHmiTheme accepts only generated themes; a probe theme reaches every screen.
    assert.strictEqual(await evaluate("setHmiTheme('no-such-theme')+'|'+setHmiTheme(HMI_THEMES[0].id)+'|'+document.documentElement.dataset.hmiTheme"), 'false|true|' + await evaluate('HMI_THEMES[0].id'));
    const themeProbe = JSON.parse(await evaluate(THEME_PROBE));
    assert.deepStrictEqual(themeProbe.problems, [], 'colours not following the theme');
    assert(themeProbe.checked > 3000, 'theme probe checked only ' + themeProbe.checked + ' colours');
    // Language: setHmiLanguage accepts only catalogue languages; a pseudo language reaches every screen.
    assert.strictEqual(await evaluate("setHmiLanguage('xx')+'|'+setHmiLanguage(HMI_LANGUAGES[0].tag)+'|'+document.querySelector('.screen').lang"), 'false|true|' + await evaluate('HMI_LANGUAGES[0].tag'));
    assert.deepStrictEqual(JSON.parse(await evaluate(PSEUDO_PROBE)), [], 'text outside the string catalogue');
    assert.strictEqual(await evaluate(PRESENTATION_CHOICE), 'DIV,DIV|BUTTON,BUTTON|xx:true|probe|en|dark');

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
    // Fullscreen: the website bar goes and the control panel becomes a side menu behind a handle.
    assert.strictEqual(await evaluate('(()=>{const shell=document.getElementById(\'demo-shell\'),panel=document.getElementById(\'control-panel\');return shell.classList.contains(\'is-full\')&&getComputedStyle(document.querySelector(\'.site-bar\')).display===\'none\'&&getComputedStyle(panel).visibility===\'hidden\'&&getComputedStyle(document.getElementById(\'drawer-toggle\')).display!==\'none\'})()'), true);
    await evaluate('document.getElementById(\'drawer-toggle\').click()');
    await sleep(400);
    assert.strictEqual(await evaluate('getComputedStyle(document.getElementById(\'control-panel\')).visibility'), 'visible');
    await evaluate('document.getElementById(\'drawer-close\').click()');
    assert.strictEqual(await evaluate('document.getElementById(\'demo-shell\').classList.contains(\'drawer-open\')'), false);
    assert.strictEqual(exceptions.length, 0, JSON.stringify(exceptions));
    console.log('PASS: HTML demo viewport, control panel, playback, scenarios and controller alerts, keyboard, dialog, theme switch, pseudo language, mobile fit, fullscreen side menu and exception checks.');
  } finally {
    if (ws) ws.close();
    if (chrome) chrome.kill();
    server.close();
  }
})().catch(error => {
  console.error(error.stack || error);
  process.exitCode = 1;
});
