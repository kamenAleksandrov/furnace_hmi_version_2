// SPDX-License-Identifier: Apache-2.0
//
// The web demo in headless Chrome over its DevTools protocol, shared by the demo's
// checks: tools/hmi_web_demo_p1_smoke.cjs and tools/hmi_web_demo_workflows.cjs
// (Phase 21 M21-8). It serves docs/images/theme/hmi-web-demo.html, opens it at the
// given viewport and hands the check an evaluate/waitFor/call interface. Windows,
// with Chrome at its default location.

const assert = require('assert');
const fs = require('fs');
const http = require('http');
const path = require('path');
const {spawn} = require('child_process');

const root = path.resolve(__dirname, '..');
const source = path.join(root, 'docs', 'images', 'theme', 'hmi-web-demo.html');
const chromePath = 'C:/Program Files/Google/Chrome/Application/chrome.exe';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// Runs check({evaluate, waitFor, call, exceptions, sleep}) on the loaded demo page.
// `profile` names the browser profile directory under build/; `debugPort` must
// differ between checks that may run at the same time.
async function withDemoPage({profile, debugPort, width = 1024, height = 720}, check) {
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
  let chrome;
  let ws;
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const httpPort = server.address().port;
  chrome = spawn(chromePath, [
    '--headless=new',
    '--no-sandbox',
    '--disable-gpu',
    '--no-first-run',
    '--no-default-browser-check',
    '--remote-debugging-address=127.0.0.1',
    '--remote-debugging-port=' + debugPort,
    '--user-data-dir=' + path.join(root, 'build', profile),
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
      width, height, deviceScaleFactor: 1, mobile: false
    });
    await call('Page.navigate', {
      url: 'http://127.0.0.1:' + httpPort + '/hmi-web-demo.html'
    });
    await waitFor('document.readyState === \'complete\' && !!document.querySelector(\'.demo-shell\')');
    await check({evaluate, waitFor, call, exceptions, sleep});
  } finally {
    if (ws) ws.close();
    if (chrome) chrome.kill();
    server.close();
  }
}

module.exports = {withDemoPage, sleep};
