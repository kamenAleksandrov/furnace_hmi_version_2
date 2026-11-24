// SPDX-License-Identifier: Apache-2.0
//
// Phase 21 M21-8 (WI-H07), the web demo's half: the M3 operator workflows of
// tests/zephyr/operator_workflows, run in the web demo against its simulated
// controller (owner 2026-11-24: the same scenarios in both presentations). Each
// step clicks what an operator clicks, found by its catalogue text, and checks
// what the demo shows. This checks the parity of the presentation, not
// controller truth (ADR-0004): the demo's controller is a simulation inside the
// page. Its scenarios stand in for the controller's scenario channel, and
// advanceDemo for the passing of time. Windows, with Chrome.
//
// Usage: node tools/hmi_web_demo_workflows.cjs

const assert = require('assert');
const {withDemoPage, sleep} = require('./hmi_web_demo_browser.cjs');

// In the page: buttons of the open dialog, else of the screen, found as an operator reads them.
const HELPERS = `window.wf={
 vis:b=>b.getClientRects().length>0&&!b.closest('[inert]'),
 dialog(){return document.getElementById('message');},
 scope(){const m=this.dialog();return m.open?m:document.querySelector('.screen');},
 label(b){return b.textContent.trim()||b.getAttribute('aria-label')||'';},
 find(text){const bs=[...this.scope().querySelectorAll('button')].filter(this.vis);
  return bs.find(b=>this.label(b)===text||b.getAttribute('aria-label')===text)||bs.find(b=>this.label(b).startsWith(text));},
 offered(text){const b=this.find(text);return !!b&&!b.disabled;},
 click(text){const b=this.find(text);if(!b)throw new Error('no button "'+text+'" on '+(this.dialog().open?'the dialog':mainPage));
  if(b.disabled)throw new Error('"'+text+'" is not offered');b.click();return true;},
 tap(id,...a){return this.click(t(id,...a));},
 title(){return this.dialog().open?document.getElementById('message-title').textContent:'';},
 shows(text){return this.scope().textContent.includes(text);},
 type(text){for(const c of text)this.click(c);return true;},
 rail(id,optional){const b=[...document.querySelectorAll('.nav button')].filter(this.vis).find(x=>this.label(x)===t(id));
  if(!b){if(optional)return false;throw new Error('no rail button '+id);}if(!b.disabled)b.click();return true;}
};true`;

const LONG_NAME = '1234567890123456789012345678901'; // 31 characters

async function workflows({evaluate, waitFor, exceptions}) {
  const tap = (id, ...args) => evaluate(`wf.tap(${JSON.stringify(id)}${args.map(a => ',' + JSON.stringify(a)).join('')})`);
  const page = () => evaluate('mainPage');
  const title = () => evaluate('wf.title()');
  const step = name => console.log('workflow: ' + name);
  const closeDialog = () => evaluate("wf.dialog().open&&wf.tap('UI_LAB_CLOSE')");
  // Every fault answered from the Faults dialog, reached from the Home banner: Acknowledge each,
  // then Recover each, as tests/zephyr/operator_workflows answer_faults does.
  const answerFaults = async () => {
    if (!(await evaluate('demoFaults.length'))) return;
    await evaluate("wf.click(HMI_STRINGS.en.UI_LAB_FAULTS_BANNER_FORMAT.split('%')[0])");
    assert(await evaluate("wf.shows(t('UI_LAB_FAULTS_TITLE'))"), 'the banner does not open the Faults dialog');
    for (let taps = 0; taps < 20; taps += 1) {
      if (await evaluate("wf.offered(t('UI_LAB_FAULT_ACKNOWLEDGE'))")) await tap('UI_LAB_FAULT_ACKNOWLEDGE');
      else if (await evaluate("wf.offered(t('UI_LAB_FAULT_RECOVER'))")) await tap('UI_LAB_FAULT_RECOVER');
      else break;
      await sleep(300);
    }
    assert.strictEqual(await evaluate('demoFaults.filter(f=>f.latched||!f.acknowledged).length'), 0,
                       'faults left after answering them');
    await closeDialog();
  };
  // The demo's answer to a library change: its dialog, closed after it opens.
  const answered = async () => {
    await waitFor('wf.dialog().open&&!controller.pending');
    await closeDialog();
  };

  await evaluate('skipDemoIntro()');
  await waitFor('!document.getElementById("demo-intro")');
  await evaluate(HELPERS);

  // W1 Program edit and save: a new program of 31 characters, saved, edited, deleted.
  step('W1 program edit and save');
  await evaluate("wf.rail('DASHBOARD_PROGRAMS')");
  await tap('UI_LAB_NEW_DRAFT');
  await tap('UI_LAB_NEW_PROGRAM_NAME');
  await tap('UI_LAB_KEYBOARD_CLEAR');
  await evaluate(`wf.type(${JSON.stringify(LONG_NAME)})`);
  await tap('UI_LAB_SETTINGS_APPLY');
  await tap('UI_LAB_ADD_STAGE_ACTION');
  await tap('UI_LAB_SAVE_DRAFT'); // the stage editor's "Save all changes", then the program's Save
  await tap('UI_LAB_SAVE_PROGRAM');
  await waitFor(`programCatalogue.some(p=>!p.deleted&&p.name===${JSON.stringify(LONG_NAME)})`);
  await answered();
  assert.strictEqual(await page(), 'programs', 'a new program does not return to Programs');
  const saved = await evaluate(`programCatalogue.findIndex(p=>!p.deleted&&p.name===${JSON.stringify(LONG_NAME)})`);
  // Opened by its row, edited with a Hold stage, saved again.
  await evaluate(`wf.click(${JSON.stringify(LONG_NAME.slice(0, 16))})`);
  await tap('UI_LAB_EDIT_PROGRAM');
  await tap('UI_LAB_ADD_STAGE_ACTION');
  await tap('UI_LAB_STAGE_HOLD');
  await tap('UI_LAB_SAVE_DRAFT');
  await tap('UI_LAB_SAVE_PROGRAM');
  await waitFor(`programCatalogue[${saved}].stages.map(s=>s.kind).join()==='Heating,Hold'`);
  await answered();
  await evaluate("wf.rail('DASHBOARD_PROGRAMS')");
  await evaluate(`wf.click(${JSON.stringify(LONG_NAME.slice(0, 16))})`);
  await tap('UI_LAB_DELETE_DRAFT');
  await tap('UI_LAB_DELETE_DRAFT');
  await waitFor(`!programCatalogue.some(p=>!p.deleted&&p.name===${JSON.stringify(LONG_NAME)})`);
  await answered();

  // W2 to W5: a hot start from a hot furnace, Pause and Resume, contained navigation, Stop and cooling.
  step('W2-W5 hot start, Pause, Resume, Stop');
  await evaluate("wf.rail('DASHBOARD_PROGRAMS')");
  await evaluate("wf.click(programCatalogue[1].name)");
  await tap('UI_LAB_START_PROGRAM');
  await tap('UI_LAB_START_PROGRAM');
  await waitFor("mainPage==='running'&&!!demoRun&&!controller.pending");
  await evaluate('advanceDemo(90)'); // the demo furnace reaches its 120 C Hold
  await tap('UI_LAB_STOP_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_START_AGAIN'))");
  await tap('UI_LAB_START_AGAIN');
  await waitFor('proposalShown()');
  // The demo's hot start: Start again on its hot, cooling furnace (it has no hot furnace at rest).
  assert(await evaluate("wf.shows(t('UI_LAB_START_PROPOSAL_TITLE_FORMAT',programCatalogue[1].name))"),
         'the proposal does not name the program');
  assert(await evaluate("wf.dialog().textContent.includes(t('UI_LAB_START_SKIPPED_HEATING_FORMAT',1,'').split(' ').slice(0,3).join(' '))"),
         'the hot start does not show the skipped stage');
  await tap('UI_LAB_START_CONFIRM');
  await waitFor("mainPage==='running'&&!!demoRun&&!demoRun.stopped&&!controller.pending");
  // W5: the library cannot be reached while the run executes.
  await evaluate("wf.rail('DASHBOARD_PROGRAMS',true)"); // not offered, or offered and refused
  assert.strictEqual(await page(), 'running', 'Programs is reachable during a run');
  // W3: Pause and Resume.
  await tap('UI_LAB_PAUSE_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_RESUME_PREVIEW'))");
  await tap('UI_LAB_RESUME_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_PAUSE_PREVIEW'))");
  // W4: Stop; the run cools on Running; Return Home leaves it cooling.
  await tap('UI_LAB_STOP_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_RETURN_HOME'))");
  await tap('UI_LAB_RETURN_HOME');
  assert.strictEqual(await page(), 'home', 'Return Home does not show Home');
  assert.strictEqual(await evaluate("!!demoRun&&demoRun.phase==='cooling'"), true, 'Return Home ended the cooling');
  await evaluate('resetDemoState();render()');

  // W9 Manual: Start, a retarget, Pause, Resume, Stop, Exit.
  step('W9 Manual');
  await evaluate("wf.rail('DASHBOARD_HOME')");
  await tap('UI_LAB_START_MANUAL');
  await tap('UI_LAB_START');
  await waitFor("!!demoRun&&demoRun.mode==='manual'&&!controller.pending");
  const target = await evaluate('demoRun.target');
  await evaluate("wf.click(t('UI_LAB_INCREASE_LABEL')+' '+t('UI_LAB_MANUAL_TARGET'))");
  await waitFor(`demoRun.target===${target + 1}&&!controller.pending`);
  await tap('UI_LAB_PAUSE_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_RESUME_PREVIEW'))");
  await tap('UI_LAB_RESUME_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_PAUSE_PREVIEW'))");
  await tap('UI_LAB_STOP_PREVIEW');
  await waitFor("wf.offered(t('UI_LAB_MANUAL_EXIT'))");
  await tap('UI_LAB_MANUAL_EXIT');
  assert.strictEqual(await page(), 'home', 'Exit does not leave the session');
  await evaluate('resetDemoState();render()');

  // W6 Quality decision and Continue: the decision panel, Continue, the run goes on.
  step('W6 quality decision');
  await evaluate("playScenario('decision')");
  await waitFor("wf.offered(t('UI_LAB_DECISION_CONTINUE'))", 600);
  await tap('UI_LAB_DECISION_CONTINUE');
  await waitFor("!wf.dialog().open||!wf.find(t('UI_LAB_DECISION_CONTINUE'))");
  assert.strictEqual(await evaluate("!!demoRun&&!demoRun.stopped&&demoRun.phase!=='fault'"), true,
                     'the run does not go on after Continue');
  await evaluate('stopScenario();resetDemoState();render()');

  // W7 Faults: a sensor fault, its alert, the Faults dialog, Stop, then the faults answered from Home.
  step('W7 sensor fault');
  await evaluate("playScenario('sensor-fault')");
  await waitFor("!!demoRun&&demoRun.phase==='fault'&&wf.dialog().open", 900);
  await closeDialog();
  assert(!(await evaluate("wf.offered(t('UI_LAB_PAUSE_PREVIEW'))")), 'a faulted run offers Pause');
  await evaluate("wf.click(t('UI_LAB_FAULTS_BUTTON_FORMAT',demoFaults.length))");
  assert(await evaluate("wf.shows(t('UI_LAB_FAULTS_TITLE'))&&wf.shows(t('UI_LAB_FAULT_3'))"),
         'the sensor fault is not listed');
  await closeDialog();
  await evaluate('stopScenario()');
  await tap('UI_LAB_STOP_PREVIEW');
  // Known parity gap (M21-8): the demo ends a stopped faulted run at once and shows Home; the
  // controller stops it into its final cooling, which the screens show until Exit or Return Home.
  await waitFor("!demoRun&&mainPage==='home'");
  await answerFaults();
  await evaluate('resetDemoState();render()');

  // W10 Link loss and reconnection during a run: lost, then restored, Stop offered again.
  step('W10 link loss');
  await evaluate("playScenario('link-lost')");
  await waitFor('demoLinkLost', 300);
  assert.strictEqual(await title(), await evaluate("t('UI_LAB_ALERT_LINK_LOST_TITLE')"));
  const lostRun = await evaluate('demoRun&&demoRun.program.id');
  await closeDialog();
  assert(!(await evaluate("wf.offered(t('UI_LAB_STOP_PREVIEW'))")), 'Stop is offered while the link is lost');
  await waitFor('!demoLinkLost && !scenarioPlayer', 600);
  // The demo opens the alert again as "Connection restored" when the link returns.
  if (await evaluate('wf.dialog().open')) {
    assert.strictEqual(await title(), await evaluate("t('UI_LAB_ALERT_LINK_RESTORED_TITLE')"));
    await closeDialog();
  }
  assert.strictEqual(await evaluate('demoRun&&demoRun.program.id'), lostRun, 'another run after the link returns');
  await tap('UI_LAB_STOP_PREVIEW');
  await waitFor("!!demoRun&&demoRun.stopped");
  await evaluate('resetDemoState();render()');

  // W8 Protective trip. Known parity gap (M21-8): the demo simulates no protective trip (fault 7)
  // and no external reset; its nearest scenario, overheating, raises the over-temperature fault (16),
  // which the operator recovers. Checked here: that fault, its Faults dialog, Stop and the answers.
  step('W8 over-temperature (no protective trip in the demo)');
  await evaluate("playScenario('overtemperature')");
  await waitFor("!!demoRun&&demoRun.phase==='fault'&&wf.dialog().open", 900);
  await closeDialog();
  await evaluate("wf.click(t('UI_LAB_FAULTS_BUTTON_FORMAT',demoFaults.length))");
  assert(await evaluate("wf.shows(t('UI_LAB_FAULT_16'))"), 'the over-temperature fault is not listed');
  await closeDialog();
  await evaluate('stopScenario()');
  await tap('UI_LAB_STOP_PREVIEW');
  await waitFor("!demoRun&&mainPage==='home'");
  await answerFaults();
  await evaluate('resetDemoState();render()');

  // W11 Device log: the log shows the session's records.
  step('W11 Device log');
  await evaluate("wf.rail('DASHBOARD_DEVICE')");
  await tap('UI_LAB_DEVICE_LOG');
  assert(await evaluate("!wf.shows(t('UI_LAB_DEVICE_UNAVAILABLE'))&&!wf.shows(t('UI_LAB_DEVICE_LOG_EMPTY'))&&!wf.shows(t('CTRL_LOG_EMPTY'))"),
         'the Device log is unavailable or empty');

  assert.strictEqual(exceptions.length, 0, JSON.stringify(exceptions));
  console.log('PASS: web demo M3 operator workflows W1 to W11 (parity with tests/zephyr/operator_workflows).');
}

withDemoPage({profile: 'm218-workflows-profile', debugPort: 9243}, workflows).catch(error => {
  console.error(error.stack || error);
  process.exitCode = 1;
});

