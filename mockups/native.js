'use strict';

(() => {
  const bridge = window.__JUCE__?.backend;
  if (!bridge) return;

  const root = document.documentElement;
  root.classList.add('juce-hosted');
  document.body.dataset.concept = 'wood';

  let current;
  let structure;
  let layerStructure;
  let templateStructure;
  let presetStructure;
  let libraryCategory = 'Dynamics';
  let audioState;
  let vstCatalog = [];
  let vstView = 'full';
  const expandedVstFolders = new Set();
  let drag;
  let lastTempoTap = 0;
  const visualViews = ['default', '01', '02', '03'];
  let visualView = visualViews.includes(localStorage.getItem('looper.visualView'))
    ? localStorage.getItem('looper.visualView') : 'default';
  let visualPage = 'loop';
  let expandedPedal = null;
  let renderVersion = 0;
  let pendingFocus = null;
  let showCables = localStorage.getItem('looper.showCables') === 'true';
  const footer = {
    input: { min: -24, max: 24, unit: 'dB' },
    gate: { min: -80, max: 0, unit: 'dB' },
    transpose: { min: -12, max: 12, unit: 'st' },
    doubler: { min: 0, max: 50, unit: 'ms' },
    output: { min: -24, max: 24, unit: 'dB' }
  };

  function act(action, fields = {}) {
    bridge.emitEvent('native-action', { action, ...fields });
  }

  function escapeHtml(value) {
    return String(value).replace(/[&<>"']/g, char => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
    })[char]);
  }

  function pedalColor(id) {
    return effects[id] ? effectColor(id) : '#c68c69';
  }

  function renderNativeLibrary(templates = []) {
    const query = document.querySelector('#effect-search').value.toLowerCase();
    const categories = [...new Set(templates.map(template => template.category))];
    if (query && !templates.some(template => template.category === libraryCategory
      && `${template.name} ${template.category} ${template.knobs.join(' ')}`.toLowerCase().includes(query)))
      libraryCategory = templates.find(template =>
        `${template.name} ${template.category} ${template.knobs.join(' ')}`.toLowerCase().includes(query))?.category ?? '';
    document.querySelector('#effect-library').innerHTML = categories.map(category => {
      const pedals = templates.filter(template => template.category === category
        && (!query || `${template.name} ${template.category} ${template.knobs.join(' ')}`.toLowerCase().includes(query)));
      if (query && !pedals.length) return '';
      const id = category.toLowerCase().replace(/[^a-z0-9]+/g, '-');
      const expanded = category === libraryCategory;
      return `<section class="library-category">
        <button class="library-folder" data-native-category="${escapeHtml(category)}" aria-expanded="${expanded}" aria-controls="native-folder-${id}">
          <span aria-hidden="true">${expanded ? '▾' : '▸'}</span>${escapeHtml(category)}<small>${pedals.length}</small>
        </button>
        <div id="native-folder-${id}" class="library-pedals" ${expanded ? '' : 'hidden'}>
          ${pedals.map(template => `<div class="library-pedal" draggable="true" data-native-template="${escapeHtml(template.id)}"
            title="Drag ${escapeHtml(template.name)} onto the pedalboard">
            <span class="effect-swatch" style="background:${pedalColor(template.id)}" aria-hidden="true"></span>
            <span class="library-pedal-name"><strong>${escapeHtml(template.name)}</strong>
              <small>${template.knobs.map(escapeHtml).join(' · ')}</small></span>
          </div>`).join('')}
        </div>
      </section>`;
    }).join('') || '<p class="library-empty">No matching pedals.</p>';
  }

  function renderVstCatalog(catalog = {}) {
    vstCatalog = [...(catalog.plugins ?? [])].sort((a, b) =>
      String(a.name || '').localeCompare(String(b.name || ''), undefined, { sensitivity: 'base' })
      || String(a.manufacturer || '').localeCompare(String(b.manufacturer || ''), undefined, { sensitivity: 'base' }));
    renderVstPickerList();
  }

  function renderVstChoice(plugin) {
    return `<button class="vst-choice" data-native-action="select-vst" data-vst-index="${plugin.index}"
      ${current?.vstPickerLoading ? 'disabled' : ''}>
      <span>${escapeHtml(plugin.name)}</span><small>${escapeHtml(plugin.manufacturer || 'Unknown manufacturer')}</small>
    </button>`;
  }

  function renderVstPickerList() {
    const list = document.querySelector('#vst-picker-list');
    const query = document.querySelector('#vst-search').value.trim().toLowerCase();
    const matches = vstCatalog.filter(plugin =>
      `${plugin.name} ${plugin.manufacturer || ''}`.toLowerCase().includes(query));
    list.setAttribute('aria-labelledby', `vst-tab-${vstView}`);
    if (!matches.length) {
      list.innerHTML = `<p class="vst-empty">${query ? 'No matching VST3 effects.' : 'No VST3 audio effects found.'}</p>`;
      return;
    }

    if (vstView === 'full') {
      list.innerHTML = matches.map(renderVstChoice).join('');
      return;
    }

    const groups = new Map();
    for (const plugin of matches) {
      const manufacturer = String(plugin.manufacturer || '').trim() || 'Unknown manufacturer';
      const key = manufacturer.toLowerCase();
      if (!groups.has(key)) groups.set(key, { name: manufacturer, plugins: [] });
      groups.get(key).plugins.push(plugin);
    }
    list.innerHTML = [...groups.entries()].sort((a, b) =>
      a[1].name.localeCompare(b[1].name, undefined, { sensitivity: 'base' })).map(([key, group]) => {
        const expanded = Boolean(query) || expandedVstFolders.has(key);
        return `<section class="vst-folder">
          <button class="vst-folder-toggle" data-vst-manufacturer="${escapeHtml(key)}" aria-expanded="${expanded}">
            <span class="vst-folder-name"><span aria-hidden="true">${expanded ? '▾' : '▸'}</span>${escapeHtml(group.name)}</span>
            <small>${group.plugins.length}</small>
          </button>
          <div class="vst-folder-contents" ${expanded ? '' : 'hidden'}>${group.plugins.map(renderVstChoice).join('')}</div>
        </section>`;
      }).join('');
  }

  function selectVstView(view) {
    vstView = view;
    document.querySelectorAll('[data-vst-tab]').forEach(tab => {
      const selected = tab.dataset.vstTab === view;
      tab.setAttribute('aria-selected', selected);
      tab.tabIndex = selected ? 0 : -1;
    });
    renderVstPickerList();
  }

  function selectSettingsTab(tab) {
    document.querySelectorAll('[data-settings-tab]').forEach(button => {
      const selected = button.dataset.settingsTab === tab;
      button.setAttribute('aria-selected', selected);
      button.tabIndex = selected ? 0 : -1;
    });
    document.querySelector('#audio-panel').hidden = tab !== 'audio';
    document.querySelector('#visual-panel').hidden = tab !== 'visual';
  }

  function updateVisualSettings() {
    document.documentElement.dataset.visualView = visualView;
    document.body.dataset.study = visualView === 'default' ? '' : `board-${Number(visualView)}`;
    document.querySelector('#visual-view').value = visualView === 'default' ? 'default' : `board-${Number(visualView)}`;
    const cables = document.querySelector('#demo-cables-toggle');
    cables.setAttribute('aria-pressed', showCables);
    cables.textContent = showCables ? 'Hide cables' : 'Show cables';
    const pages = document.querySelector('#study-pages');
    pages.hidden = visualView !== '03';
    pages.querySelectorAll('[data-study-page]').forEach(button =>
      button.setAttribute('aria-pressed', button.dataset.studyPage === visualPage));
  }

  function slotForVisibleCards(slot) {
    if (visualView !== '03') return slot;
    if (visualPage === 'post') return current.deckIndex + 1 + slot;
    if (visualPage === 'loop') return current.deckIndex + 1;
    return slot;
  }

  function visibleSlotAtPoint(cards, x, y) {
    const target = cards.reduce((closest, item, index) => {
      const bounds = item.getBoundingClientRect();
      const distance = Math.hypot(x - (bounds.left + bounds.width / 2),
        y - (bounds.top + bounds.height / 2));
      return distance < closest.distance ? { index, distance, centerX: bounds.left + bounds.width / 2 } : closest;
    }, { index: -1, distance: Infinity, centerX: 0 });
    return target.index < 0 ? cards.length : target.index + (x > target.centerX ? 1 : 0);
  }

  function setLibraryOpen(open, focus = false) {
    document.body.classList.toggle('toolbox-open', open);
    document.querySelector('#wood-toolbox').setAttribute('aria-expanded', open);
    if (focus) document.querySelector(open ? '#effect-search' : '#wood-toolbox').focus();
  }

  document.querySelector('#layers-dialog').insertAdjacentHTML('beforeend', `
    <section class="loop-files" aria-label="Loop files">
      <div class="loop-file-buttons">
        <button data-native-action="save-project">Save loop</button>
        <button data-native-action="load-project">Open loop</button>
        <button data-native-action="import-wav">Import WAV</button>
        <button data-native-action="export-wav">Export WAV</button>
      </div>
      <p>Stop transport to use files. Loop projects keep all layers and mixer settings.
        WAV import replaces the loop; WAV export mixes enabled/soloed layers without live input, click, or board effects.</p>
      <p id="loop-file-status" role="status"></p>
    </section>`);

  document.querySelector('#audio-panel').innerHTML = `<div class="audio-settings">
    <div class="audio-heading"><div><span class="eyebrow">AUDIO DEVICE</span>
      <h3 id="native-device-heading"></h3></div><button data-audio-action="refresh">Refresh</button></div>
    <p id="native-audio-error" class="audio-error" role="alert" hidden></p>
    <p id="native-audio-lock" class="audio-lock" hidden>Device and MIDI settings are locked while a loop exists. Clear the loop to change them.</p>
    <label class="audio-field">Input mode<select id="native-input-mode">
      <option value="0">Mono: first input (centered)</option>
      <option value="1">Mono: second input (centered)</option>
      <option value="2">Stereo: two selected inputs</option>
    </select></label>
    <fieldset id="audio-device-controls" disabled>
      <label class="audio-field">Audio device type<select id="audio-type" data-audio-setting="type"></select></label>
      <div class="audio-pair">
        <label class="audio-field"><span id="audio-output-label">Output</span><select id="audio-output" data-audio-setting="output"></select></label>
        <button data-audio-action="test" title="Play a test tone">Test</button>
      </div>
      <div id="audio-input-row" class="audio-pair">
        <label id="audio-input-field" class="audio-field">Input<select id="audio-input" data-audio-setting="input"></select></label>
        <span id="audio-level-label" hidden>Input level</span>
        <meter id="audio-input-meter" min="0" max="1" value="0" aria-label="Input level"></meter>
      </div>
      <div class="audio-channel-grid">
        <div class="audio-field">Active output channels<div class="audio-options" id="audio-output-channels"></div></div>
        <div class="audio-field">Active input channels<div class="audio-options" id="audio-input-channels"></div></div>
      </div>
      <div class="audio-channel-grid">
        <label class="audio-field">Sample rate<select id="audio-rate" data-audio-setting="rate"></select></label>
        <label class="audio-field">Audio buffer size<select id="audio-buffer" data-audio-setting="buffer"></select></label>
      </div>
      <div class="audio-field">Active MIDI inputs<div class="audio-options" id="audio-midi"></div></div>
      <div class="audio-buttons" id="audio-driver-buttons" hidden>
        <button data-audio-action="controlPanel">Control panel</button><button data-audio-action="reset">Reset device</button>
      </div>
    </fieldset>
  </div>`;
  function updateAudio(state) {
    audioState = state;
    const select = (id, entries, value, allowNone = false) => {
      const control = document.getElementById(id);
      const options = (allowNone ? [['', 'None']] : []).concat(entries);
      const markup = options.map(([key, label]) => `<option value="${escapeHtml(key)}">${escapeHtml(label)}</option>`).join('');
      if (control.innerHTML !== markup) control.innerHTML = markup;
      control.value = String(value);
      control.disabled = options.length === 0;
    };
    const channels = (id, entries, name) => {
      const container = document.getElementById(id);
      const key = JSON.stringify(entries.map(item => [item.id, item.name]));
      if (container.dataset.entries !== key) {
        container.dataset.entries = key;
        container.innerHTML = entries.length ? entries.map((item, index) =>
          `<label><input type="checkbox" data-audio-setting="${name}" data-index="${index}" data-id="${escapeHtml(item.id ?? '')}"><span>${escapeHtml(item.name)}</span></label>`
        ).join('') : '<span class="audio-empty">None available</span>';
      }
      container.querySelectorAll('input').forEach((input, index) => { input.checked = entries[index].active; });
    };
    select('audio-type', state.types.map(name => [name, name]), state.type);
    select('audio-output', state.outputs.map(name => [name, name]), state.output, true);
    select('audio-input', state.inputs.map(name => [name, name]), state.input, true);
    select('audio-rate', state.rates.map(rate => [rate, `${rate} Hz`]), state.rate);
    select('audio-buffer', state.buffers.map(size => [size, `${size} samples (${state.rate ? (size * 1000 / state.rate).toFixed(1) : '0.0'} ms)`]), state.buffer);
    channels('audio-output-channels', state.outputChannels, 'outputChannel');
    channels('audio-input-channels', state.inputChannels, 'inputChannel');
    channels('audio-midi', state.midiInputs, 'midi');
    document.querySelector('#audio-input-field').hidden = !state.separate;
    document.querySelector('#audio-level-label').hidden = state.separate;
    document.querySelector('#audio-output-label').textContent = state.separate ? 'Output' : 'Device';
    document.querySelector('#audio-driver-buttons').hidden = !state.controlPanel;
    const error = document.querySelector('#native-audio-error');
    error.textContent = state.error;
    error.hidden = !state.error;
    if (current) updateAudioLock(current);
  }

  function updateAudioLock(state) {
    document.querySelector('#audio-device-controls').disabled = state.settingsLocked || !audioState;
    document.querySelector('#native-audio-lock').hidden = !state.settingsLocked;
    document.querySelector('#native-device-heading').textContent = state.deviceReady ? 'Audio device connected.' : 'No device connected.';
    document.querySelector('#audio-input-meter').value = Math.max(0, Math.min(1, state.inputPeak));
    document.querySelector('[data-audio-action="test"]').disabled = !state.deviceReady;
  }

  function renderLayers(state) {
    const colors = ['#bf925f', '#a89ba4', '#b6a58b', '#9e8b82', '#9394a6', '#c4a581', '#bb8a8a', '#aca39a'];
    const bars = Array.from({ length: 26 }, (_, i) =>
      `<i style="--height:${5 + (i * 13 % 23)}px"></i>`).join('');
    document.querySelector('#layers').innerHTML = state.layers.map((layer, index) => `
      <div class="layer" style="--layer-color:${colors[index % colors.length]}" data-native-layer="${layer.id}">
        <div class="layer-id"><span>${String(index + 1).padStart(2, '0')}</span>
          <div><strong>Layer ${index + 1}</strong><small class="layer-status"></small></div></div>
        <div class="waveform" aria-hidden="true">${bars}</div>
        <label class="layer-slider"><span>LEVEL <output></output></span><input data-volume="${layer.id}" aria-label="Layer ${index + 1} volume" type="range" min="0" max="100"></label>
        <label class="layer-slider"><span>PAN <output></output></span><input data-pan="${layer.id}" aria-label="Layer ${index + 1} pan" type="range" min="-100" max="100"></label>
        <div class="layer-actions">
          <button data-native-layer-action="active" aria-label="Toggle layer ${index + 1}"></button>
          <button data-native-layer-action="solo" aria-label="Solo layer ${index + 1}">SOLO</button>
          <button data-native-layer-action="retake" aria-label="Retake layer ${index + 1}">RETAKE</button>
          <button data-native-layer-action="delete" aria-label="Delete layer ${index + 1}">DELETE</button>
        </div>
      </div>`).join('');
  }

  function updateDialogs(state) {
    for (const action of ['save-project', 'export-wav', 'load-project', 'import-wav']) {
      document.querySelector(`[data-native-action="${action}"]`).disabled =
        action === 'save-project' || action === 'export-wav' ? !state.canSaveLoop : !state.canLoadLoop;
    }
    document.querySelector('#loop-file-status').textContent = state.error || state.loopFileStatus || '';
    const key = state.layers.map(layer => layer.id).join(',');
    if (key !== layerStructure) {
      layerStructure = key;
      renderLayers(state);
    }
    document.querySelector('#layer-count').textContent = `${state.layers.length} / ${state.layered ? 8 : 1}`;
    document.querySelector('#empty-layers').hidden = state.layers.length !== 0;
    for (const layer of state.layers) {
      const row = document.querySelector(`[data-native-layer="${layer.id}"]`);
      row.classList.toggle('inactive', !layer.active);
      row.classList.toggle('soloed', layer.solo);
      row.querySelector('.layer-status').textContent = layer.capturing ? 'TAKE / QUEUED' : `RECORDED · ${state.duration.toFixed(1)}s`;
      for (const name of ['volume', 'pan']) {
        const input = row.querySelector(`[data-${name}]`);
        if (document.activeElement !== input) input.value = layer[name];
        const value = Number(input.value);
        input.closest('label').querySelector('output').textContent = name === 'volume' ? `${value}%`
          : value === 0 ? 'C' : `${value < 0 ? 'L' : 'R'}${Math.abs(value)}`;
      }
      const active = row.querySelector('[data-native-layer-action="active"]');
      active.textContent = layer.active ? 'ON' : 'OFF';
      active.setAttribute('aria-pressed', layer.active);
      row.querySelector('[data-native-layer-action="solo"]').setAttribute('aria-pressed', layer.solo);
      for (const name of ['retake', 'delete'])
        row.querySelector(`[data-native-layer-action="${name}"]`).disabled = !state.canEditLayers;
    }
    const layeredButton = document.querySelector('#layered-setting');
    layeredButton.disabled = state.settingsLocked;
    layeredButton.setAttribute('aria-pressed', state.layered);
    layeredButton.title = `Layered loop ${state.layered ? 'on' : 'off'}`;
    const metronomeButton = document.querySelector('#click-setting');
    metronomeButton.setAttribute('aria-pressed', state.metronome);
    metronomeButton.title = `Metronome ${state.metronome ? 'on' : 'off'}`;
    document.querySelector('#tempo-tap').disabled = state.countingIn;
    const settings = {
      'bpm-setting': [state.bpm, state.countingIn],
      'beats-setting': [state.beats, state.countingIn],
      'unit-setting': [state.unit, state.countingIn],
      'count-setting': [state.countIn, state.settingsLocked],
      'loop-bars-setting': [state.loopBars, state.settingsLocked],
      'native-input-mode': [state.inputMode, !state.canChangeInput]
    };
    for (const [id, [value, disabled]] of Object.entries(settings)) {
      const control = document.getElementById(id);
      control.disabled = disabled;
      if (document.activeElement !== control) {
        if (control.type === 'checkbox') control.checked = value;
        else control.value = id === 'bpm-setting' ? Number(value).toFixed(2) : value;
      }
    }
    const vstPicker = document.querySelector('#vst-picker');
    const vstTarget = state.effects.find(effect => effect.id === state.vstTarget);
    if (state.vstTarget) {
      document.querySelector('#vst-picker-title').textContent = state.vstReplacing
        ? `Change VST3 on ${vstTarget?.name ?? 'pedal'}`
        : `Add a VST3 to ${vstTarget?.name ?? 'pedal'}`;
      document.querySelector('#vst-picker-message').textContent = state.vstPickerError
        || (state.vstPickerLoading ? 'Scanning installed VST3 effects…'
          : vstCatalog.length ? `${vstCatalog.length} effects · alphabetical order` : 'No VST3 audio effects found.');
      document.querySelector('#vst-refresh').disabled = Boolean(state.vstPickerLoading);
      document.querySelectorAll('#vst-picker-list .vst-choice').forEach(button => {
        button.disabled = Boolean(state.vstPickerLoading);
      });
      if (!vstPicker.open) vstPicker.showModal();
    } else if (vstPicker.open) {
      vstPicker.close();
    }
    updateAudioLock(state);
  }

  function dial(name, value, range, label, strip = false) {
    const turn = (value - range.min) * 270 / (range.max - range.min) - 135;
    return `<div class="deck-dial level-dial ${strip ? 'strip-dial' : ''}" data-level="${name}" role="slider" tabindex="0"
      aria-label="${label}" aria-valuemin="${range.min}" aria-valuemax="${range.max}" aria-valuenow="${value}"
      aria-valuetext="${value}${range.unit ? ` ${range.unit}` : '%'}" style="--turn:${turn}deg"
      title="Drag up/down or use arrow keys to adjust"></div>`;
  }

  function pedalMarkup(item, index, visualGroup) {
    if (item.type === 'looper') {
      const state = current;
      const action = state.recordLabel;
      const buttonLabel = action.toLowerCase();
      const deckControls = Object.entries(state.levels).map(([name, value]) =>
        `<div class="deck-control"><span>${name.toUpperCase()}</span>${dial(name, value, { min: 0, max: 100 }, `Deck ${name} level`)}<span class="level-value">${value}%</span></div>`
      ).join('');
      const moveHandle = visualView === 'default' || visualView === '02' ? 'data-drag="looper"' : '';
      return `<article class="pedal looper visual-deck ${state.recording ? 'recording' : ''} ${state.playing ? '' : 'is-stopped'}" data-pedal="looper">
        <i class="case-corner top-left"></i><i class="case-corner top-right"></i><i class="case-corner bottom-left"></i><i class="case-corner bottom-right"></i>
        <div class="drag-handle" ${moveHandle} tabindex="0"><div class="pedal-number"><span>0${index + 1} / LOOP ENGINE</span><span>STEREO</span></div><h3 class="pedal-title">Loop Deck</h3></div>
        <div class="deck-controls">${deckControls}</div>
        <div class="deck-face"><div class="tape-reel" aria-hidden="true"></div>
          <div class="loop-screen"><div class="progress-ring" style="--progress:${state.progress}" aria-hidden="true"></div>
            <div class="loop-readout"><strong>${escapeHtml(state.status)}</strong><span class="tempo-readout">${state.bpm}<small>BPM</small></span><span class="signature-readout">${state.beats}/${state.unit}</span></div>
          </div><div class="tape-reel" aria-hidden="true"></div>
        </div>
        <div class="footswitches"><div class="foot-group"><button class="foot-button" data-native-action="record" aria-label="${escapeHtml(buttonLabel)}"></button><span>${escapeHtml(action)}</span></div>
          <div class="foot-group"><button class="foot-button" data-native-action="stop" aria-label="Stop" ${state.canStop ? '' : 'disabled'}></button><span>STOP</span></div></div>
        <div class="looper-utility"><span>${state.layers.length} ${state.layers.length === 1 ? 'LAYER' : 'LAYERS'} · ${state.metronome ? 'CLICK ON' : 'CLICK OFF'}</span><button data-native-action="clear" ${state.canClear ? '' : 'disabled'}>CLEAR</button></div>
      </article>`;
    }

    return `<article class="pedal effect ${visualGroup} ${!item.loaded || item.bypassed ? 'off' : ''} ${item.loaded ? 'has-vst' : ''}"
      data-pedal="${escapeHtml(item.id)}" style="--pedal:${pedalColor(item.template)};--pedal-ink:${effects[item.template] ? galleryAppearance(item.template).ink : '#242521'}">
      <div class="drag-handle" data-drag="${escapeHtml(item.id)}" tabindex="0">
        <div class="pedal-number"><span>0${index + 1} / ${escapeHtml(item.category)}</span><span>${item.loaded ? (item.channels === 1 ? 'MONO' : 'STEREO') : 'TEMPLATE'}</span></div>
        <h3 class="pedal-title">${escapeHtml(item.loaded ? item.pluginName : item.name)}</h3><span class="pedal-category">${escapeHtml(item.category)}</span>
      </div>
      <div class="effect-panel ${escapeHtml(item.template)}"><span>${escapeHtml(item.loaded ? item.pluginName : item.category)}</span>
        <small>${item.loaded ? 'VST3 AUDIO EFFECT' : 'VST3 PEDAL TEMPLATE'}</small>
        <div class="vst-management">
          <button data-native-action="${item.loaded ? 'editor' : 'add-vst'}" data-effect="${escapeHtml(item.id)}">${item.loaded ? 'OPEN VST3' : 'ADD VST3'}</button>
          ${item.loaded ? `<button data-native-action="change-vst" data-effect="${escapeHtml(item.id)}">CHANGE</button>
          <button data-native-action="remove-vst" data-effect="${escapeHtml(item.id)}">REMOVE VST</button>` : ''}
        </div></div>
      <div class="pedal-knobs ${item.knobs.length === 3 ? 'three-knobs' : ''}" aria-label="${escapeHtml(item.name)} controls">${item.knobs.map((knob, index) =>
        `<div class="knob-group">
          <div class="effect-knob level-dial" data-parameter-knob="${index}" data-effect="${escapeHtml(item.id)}" role="slider"
            aria-label="${escapeHtml(knob)}" aria-valuemin="0" aria-valuemax="1000" aria-valuenow="0"
            ${item.assignments[index]?.id ? 'tabindex="0"' : 'aria-disabled="true"'}></div>
          <select class="parameter-select" data-map-knob="${index}" data-effect="${escapeHtml(item.id)}"
            aria-label="Assign ${escapeHtml(knob)} parameter" ${item.loaded ? '' : 'disabled'}>
            <option value="">Unassigned</option>${item.parameters.map(parameter =>
              `<option value="${escapeHtml(parameter.id)}" ${item.assignments[index]?.id === parameter.id ? 'selected' : ''}>${escapeHtml(parameter.name)}</option>`
            ).join('')}</select><span class="parameter-value"></span></div>`
      ).join('')}</div>
      <div class="effect-status"><span class="led"></span>${!item.loaded ? 'NO VST3' : item.bypassed ? 'BYPASSED' : 'ENGAGED'}</div>
      <button class="bypass-button" data-native-action="bypass" data-effect="${escapeHtml(item.id)}" aria-label="${item.bypassed ? 'Engage' : 'Bypass'} ${escapeHtml(item.name)}" aria-pressed="${!item.bypassed}" ${item.loaded ? '' : 'disabled'}></button>
      <span class="footswitch-label">${item.bypassed ? 'ENGAGE' : 'BYPASS'}</span>
      <button data-native-action="remove-effect" data-effect="${escapeHtml(item.id)}">REMOVE PEDAL</button>
    </article>`;
  }

  function compactPedal(item) {
    const isLooper = item.type === 'looper';
    const name = isLooper ? 'Loop Deck' : item.name;
    const artworkType = effects[item.template] ? item.template : 'boost';
    let art = isLooper
      ? document.querySelector('.gallery-looper').outerHTML.replaceAll('looper-finish', 'study-looper-finish').replaceAll('looper-metal', 'study-looper-metal')
      : galleryPedal(artworkType).replaceAll(`finish-${artworkType}`, `study-finish-${item.id}`);
    if (!isLooper && artworkType !== item.template)
      art = art.replace(/(<text class="gallery-pedal-name"[^>]*>).*?(<\/text>)/,
        `$1${escapeHtml(item.pluginName || item.name)}$2`);
    art = art.replace('<svg ', '<svg aria-hidden="true" ');
    return `<div class="study-mini-wrap" data-study-card="${escapeHtml(item.id)}">
      <div class="study-mini" data-pedal="${escapeHtml(item.id)}">${art}
        ${!isLooper || visualView === '02' ? `<button type="button" class="study-mini-handle" data-drag="${escapeHtml(item.id)}"
          aria-label="Move ${escapeHtml(name)}. Drag or use Left and Right arrow keys." title="Drag to move ${escapeHtml(name)}"></button>` : ''}
        <button type="button" class="study-mini-open" data-native-expand="${escapeHtml(item.id)}"
          aria-label="Open ${escapeHtml(name)} controls" title="Open ${escapeHtml(name)} controls"></button>
      </div></div>`;
  }

  function layoutNativeBoard(allPedals, compact) {
    const board = document.querySelector('#board');
    const order = allPedals.map(item => ({ id: item.id, type: item.type === 'looper' ? 'looper' : item.template }));
    if (compact) {
      layoutStudyBoard(order, `board-${Number(visualView)}`, visualPage);
      const origin = board.getBoundingClientRect();
      board.querySelector('.study-side').querySelectorAll('.study-row:not([hidden])').forEach((row, index) => {
        const rect = row.getBoundingClientRect();
        board.style.setProperty(`--study-rail-${index + 1}`, `${rect.top - origin.top + rect.height / 2}px`);
      });
    }
    document.querySelector('#study-tuner-cable').hidden = !showCables || !compact;
    const svg = document.querySelector('#cables');
    if (!showCables) svg.replaceChildren();
    else if (compact) drawStudyCables(order);
    else renderPatchCables(board, svg, [...document.querySelectorAll('#pedals [data-pedal]')]);
  }

  function renderExpanded(item, index) {
    const container = document.querySelector('#study-expanded-pedal');
    container.innerHTML = pedalMarkup(item, index);
    container.querySelectorAll('[data-drag]').forEach(handle => {
      handle.removeAttribute('data-drag');
      handle.removeAttribute('tabindex');
    });
  }

  function renderStructure(state) {
    const version = ++renderVersion;
    const allPedals = [{ id: 'looper', type: 'looper' }, ...state.effects];
    const index = allPedals.findIndex(pedal => pedal.id === 'looper');
    allPedals.splice(index, 1);
    allPedals.splice(state.deckIndex, 0, { id: 'looper', type: 'looper' });
    const pre = allPedals.slice(0, state.deckIndex);
    const post = allPedals.slice(state.deckIndex + 1);
    const visiblePedals = visualView !== '03' ? allPedals
      : visualPage === 'loop' ? allPedals.filter(item => item.type === 'looper')
        : visualPage === 'pre' ? pre : post;
    updateVisualSettings();
    const board = document.querySelector('#board');
    const pedals = document.querySelector('#pedals');
    const compact = visualView !== 'default' && !(visualView === '03' && visualPage === 'loop');
    const rows = items => `<div class="study-row">${items.map(compactPedal).join('')}</div><div class="study-row"></div>`;
    board.style.height = '';
    board.style.minHeight = '';
    delete board.dataset.studyRails;
    board.querySelectorAll('.study-extra-rail').forEach(rail => rail.remove());
    if (visualView === '01') {
      pedals.innerHTML = `<div class="study-side" data-study-side="pre"><span class="study-side-label">PRE / RECORDED</span>${rows(pre)}</div>
        <div class="study-center">${pedalMarkup(allPedals[state.deckIndex], state.deckIndex)}</div>
        <div class="study-side" data-study-side="post"><span class="study-side-label">POST / PLAYBACK</span>${rows(post)}</div>`;
    } else if (compact) {
      pedals.innerHTML = `<div class="study-side ${visualView === '02' ? 'study-full-chain' : 'study-paged-chain'}">${rows(visiblePedals)}</div>`;
    } else {
      pedals.innerHTML = visiblePedals.map(item => pedalMarkup(item, allPedals.indexOf(item))).join('');
    }
    if (expandedPedal) {
      const item = allPedals.find(pedal => pedal.id === expandedPedal);
      if (item) renderExpanded(item, allPedals.indexOf(item));
      else document.querySelector('#study-pedal-dialog').close();
    }
    if (pendingFocus) {
      const target = [...pedals.querySelectorAll('[data-drag], [data-native-expand]')]
        .find(element => (element.dataset.drag || element.dataset.nativeExpand) === pendingFocus);
      if (target) { target.focus({ preventScroll: true }); pendingFocus = null; }
    }
    requestAnimationFrame(() => {
      if (version !== renderVersion) return;
      layoutNativeBoard(allPedals, compact);
    });
    document.querySelector('#chain-order').innerHTML = allPedals.map(item =>
      `<span class="${item.type === 'looper' ? 'loop-order' : ''}">${escapeHtml(item.type === 'looper' ? 'LOOPER' : item.name)}</span>`
    ).join('');
    document.querySelector('#wood-layers').textContent = `Layers (${state.layers.length})`;
    document.querySelector('#wood-tuner').setAttribute('aria-pressed', state.tuner);
    document.querySelector('#wood-tuner').title = state.tunerText;
    const controls = Object.entries(state.footer).map(([name, value]) => {
      const range = footer[name];
      const text = `${value > 0 && range.min < 0 ? '+' : ''}${value} ${range.unit}`;
      const toggle = name === 'gate' || name === 'doubler';
      return `<div class="board-control"><b>${name.toUpperCase()}</b>${dial(`board-${name}`, value, range, `Board ${name}`, true)}
        <span class="level-value">${text}</span>${toggle ? `<button class="board-switch" data-board-switch="${name}" data-native-switch="${name}" role="switch" aria-label="${name}" aria-checked="${state[name]}">${state[name] ? 'ON' : 'OFF'}</button>` : ''}</div>`;
    });
    const loopControls = visualView === '02' ? `<div class="study-footer-loop" aria-label="Looper quick controls">
      <strong>LOOP DECK</strong>${Object.entries(state.levels).map(([name, value]) =>
        `<div class="board-control"><b>${name.toUpperCase()}</b>${dial(name, value, { min: 0, max: 100 }, `Deck ${name} level`, true)}<span class="level-value">${value}%</span></div>`).join('')}
      <div class="study-transport"><button type="button" data-native-action="record">${escapeHtml(state.recordLabel)}</button>
        <button type="button" data-native-action="stop" ${state.canStop ? '' : 'disabled'}>Stop</button>
        <button type="button" data-native-action="clear" ${state.canClear ? '' : 'disabled'}>Clear</button></div>
      <div class="study-timing"><label>BPM<input type="number" data-native-setting="bpm" min="30" max="300" step="1"></label>
        <label>BEATS<input type="number" data-native-setting="beats" min="1" max="16"></label>
        <label>UNIT<select data-native-setting="unit">${[1, 2, 4, 8, 16].map(unit => `<option>${unit}</option>`).join('')}</select></label></div></div>` : '';
    document.querySelector('#wood-footer').innerHTML =
      `<div class="board-controls">${controls.slice(0, 3).join('')}</div>${loopControls}<div class="board-controls">${controls.slice(3).join('')}</div>`;
    document.querySelector('#board-title').textContent = 'The loop deck';
    document.querySelector('#board-hint').textContent = state.error || '';
    setLibraryOpen(document.body.classList.contains('toolbox-open'));
    document.querySelector('#layout-reset').hidden = true;
    document.querySelector('#layer-mixer').hidden = false;
    if (state.error) {
      const toast = document.querySelector('#toast');
      toast.textContent = state.error;
      toast.classList.add('visible');
      setTimeout(() => toast.classList.remove('visible'), 3500);
    }
  }

  function updateDynamic(state) {
    document.querySelectorAll('[data-native-setting]').forEach(control => {
      if (document.activeElement !== control) control.value = state[control.dataset.nativeSetting];
      control.disabled = state.countingIn;
    });
    const quick = document.querySelector('.study-transport');
    if (quick) {
      quick.querySelector('[data-native-action="record"]').textContent = state.recordLabel;
      quick.querySelector('[data-native-action="stop"]').disabled = !state.canStop;
      quick.querySelector('[data-native-action="clear"]').disabled = !state.canClear;
    }
    document.querySelectorAll('.pedal.looper').forEach(looper => {
      const progress = looper.querySelector('.progress-ring');
      if (progress) progress.style.setProperty('--progress', state.progress);
      const screen = looper.querySelector('.loop-readout strong');
      if (screen) screen.textContent = state.status;
      const tempo = looper.querySelector('.tempo-readout');
      if (tempo) tempo.innerHTML = `${state.bpm}<small>BPM</small>`;
      const signature = looper.querySelector('.signature-readout');
      if (signature) signature.textContent = `${state.beats}/${state.unit}`;
      looper.classList.toggle('recording', state.recording);
      looper.classList.toggle('is-stopped', !state.playing);
      const footswitch = looper.querySelector('[data-native-action="record"]');
      const label = footswitch?.nextElementSibling;
      if (footswitch) footswitch.setAttribute('aria-label', state.recordLabel.toLowerCase());
      if (label) label.textContent = state.recordLabel;
      const clear = looper.querySelector('[data-native-action="clear"]');
      if (clear) clear.disabled = !state.canClear;
      const stop = looper.querySelector('[data-native-action="stop"]');
      if (stop) stop.disabled = !state.canStop;
      const layerSummary = looper.querySelector('.looper-utility > span');
      if (layerSummary) layerSummary.textContent = `${state.layers.length} ${state.layers.length === 1 ? 'LAYER' : 'LAYERS'} · ${state.metronome ? 'CLICK ON' : 'CLICK OFF'}`;
    });
    const tuner = document.querySelector('#wood-tuner');
    const note = state.tuner ? state.tunerNote : '';
    const cents = state.tunerCents;
    tuner.setAttribute('aria-pressed', state.tuner);
    tuner.setAttribute('aria-label', `Tuner: ${state.tunerText}`);
    tuner.title = state.tunerText;
    tuner.dataset.tuning = note ? (Math.abs(cents) <= 3 ? 'in-tune' : cents < 0 ? 'flat' : 'sharp') : 'idle';
    tuner.querySelector('.tuner-notes').innerHTML = tunerNotes(note.replace(/-?\d+$/, ''));
    tuner.querySelector('.tuner-label').textContent = !state.tuner ? 'TUNER · OFF'
      : !note ? 'TUNER · LISTENING'
      : `${note} · ${Math.abs(cents) <= 3 ? 'IN TUNE' : `${cents > 0 ? '+' : ''}${cents} CENTS`}`;
    for (const [name, value] of Object.entries(state.levels)) updateDial(name, value, { min: 0, max: 100 }, `${value}%`);
    document.querySelectorAll('[data-parameter-knob]').forEach(control => {
      if (drag?.control === control) return;
      const effect = state.effects.find(item => item.id === control.dataset.effect);
      const assignment = effect?.assignments[Number(control.dataset.parameterKnob)];
      const value = Math.round((assignment?.value || 0) * 1000);
      control.setAttribute('aria-valuenow', value);
      control.setAttribute('aria-valuetext', assignment?.text || 'Unassigned');
      control.style.setProperty('--turn', `${value * .27 - 135}deg`);
      control.parentElement.querySelector('.parameter-value').textContent = assignment?.text || '';
    });
    for (const [name, value] of Object.entries(state.footer)) {
      const range = footer[name];
      const text = `${value > 0 && range.min < 0 ? '+' : ''}${value} ${range.unit}`;
      updateDial(`board-${name}`, value, range, text);
      const toggle = document.querySelector(`[data-native-switch="${name}"]`);
      if (toggle) {
        toggle.setAttribute('aria-checked', state[name]);
        toggle.textContent = state[name] ? 'ON' : 'OFF';
      }
    }
  }

  function updateDial(name, value, range, text) {
    document.querySelectorAll(`[data-level="${name}"]`).forEach(control => {
      if (drag?.control === control) return;
      control.setAttribute('aria-valuenow', value);
      control.setAttribute('aria-valuetext', text);
      control.style.setProperty('--turn', `${(value - range.min) * 270 / (range.max - range.min) - 135}deg`);
      if (control.nextElementSibling?.classList.contains('level-value')) control.nextElementSibling.textContent = text;
    });
  }

  function applyState(state) {
    if (!state) return;
    current = state;
    const preset = document.querySelector('#board-preset');
    const newPresetStructure = JSON.stringify([state.boardPresets, state.boardPreset]);
    if (newPresetStructure !== presetStructure) {
      presetStructure = newPresetStructure;
      preset.replaceChildren(new Option('Unsaved board', ''));
      if (state.boardPreset) preset.add(new Option('Reload current preset', 'reload'));
      for (const name of state.boardPresets) preset.add(new Option(name, `preset:${name}`));
    }
    const selectedPreset = state.boardPreset ? `preset:${state.boardPreset}` : '';
    if (preset.value !== selectedPreset) preset.value = selectedPreset;
    const newTemplateStructure = JSON.stringify(state.pedalTemplates);
    if (newTemplateStructure !== templateStructure) {
      templateStructure = newTemplateStructure;
      renderNativeLibrary(state.pedalTemplates);
    }
    const key = JSON.stringify([
      state.effects.map(effect => [effect.id, effect.template, effect.pluginName, effect.loaded, effect.bypassed,
        effect.parameters, effect.assignments.map(assignment => assignment.id)]),
      state.layers.map(layer => [layer.id, layer.active, layer.solo, layer.capturing]),
      state.deckIndex, state.recording, state.canClear, state.error
    ]);
    if (key !== structure) {
      structure = key;
      renderStructure(state);
    }
    updateDynamic(state);
    updateDialogs(state);
  }

  function requestDial(control, value) {
    if (control.dataset.parameterKnob !== undefined) {
      value = Math.max(0, Math.min(1000, Math.round(value)));
      act('set-parameter', { effect: control.dataset.effect, knob: Number(control.dataset.parameterKnob), value: value / 1000 });
      control.setAttribute('aria-valuenow', value);
      control.style.setProperty('--turn', `${value * .27 - 135}deg`);
      return;
    }
    const name = control.dataset.level;
    if (name.startsWith('board-')) {
      const field = name.slice(6);
      const range = footer[field];
      value = Math.max(range.min, Math.min(range.max, Math.round(value)));
      act('set-footer', { name: field, value });
    } else {
      value = Math.max(0, Math.min(100, Math.round(value)));
      act('set-level', { name, value });
    }
    control.setAttribute('aria-valuenow', value);
    control.style.setProperty('--turn', `${(value - Number(control.getAttribute('aria-valuemin'))) * 270 /
      (Number(control.getAttribute('aria-valuemax')) - Number(control.getAttribute('aria-valuemin'))) - 135}deg`);
    if (control.nextElementSibling?.classList.contains('level-value')) {
      const range = name.startsWith('board-') ? footer[name.slice(6)] : null;
      control.nextElementSibling.textContent = range
        ? `${value > 0 && range.min < 0 ? '+' : ''}${value} ${range.unit}` : `${value}%`;
    }
  }

  bridge.addEventListener('native-state', applyState);
  bridge.addEventListener('audio-state', updateAudio);
  bridge.addEventListener('vst-catalog', renderVstCatalog);
  bridge.emitEvent('native-ready', {});

  document.querySelector('#vst-search').addEventListener('input', renderVstPickerList);
  document.querySelector('#settings-tabs').addEventListener('keydown', event => {
    if (event.key !== 'ArrowLeft' && event.key !== 'ArrowRight') return;
    const tabs = [...document.querySelectorAll('[data-settings-tab]')];
    const index = tabs.indexOf(event.target);
    if (index < 0) return;
    const offset = event.key === 'ArrowRight' ? 1 : -1;
    const next = tabs[(index + offset + tabs.length) % tabs.length];
    selectSettingsTab(next.dataset.settingsTab);
    next.focus();
    event.preventDefault();
  });
  document.querySelector('#vst-picker-tabs').addEventListener('keydown', event => {
    if (event.key !== 'ArrowLeft' && event.key !== 'ArrowRight') return;
    const tabs = [...document.querySelectorAll('[data-vst-tab]')];
    const index = tabs.indexOf(event.target);
    if (index < 0) return;
    const offset = event.key === 'ArrowRight' ? 1 : -1;
    const next = tabs[(index + offset + tabs.length) % tabs.length];
    selectVstView(next.dataset.vstTab);
    next.focus();
    event.preventDefault();
  });

  document.addEventListener('click', event => {
    const button = event.target.closest('button');
    if (document.body.classList.contains('toolbox-open')
      && !event.target.closest('.library, #wood-toolbox')) setLibraryOpen(false);
    if (!button) return;
    const action = button.dataset.nativeAction;
    if (action) {
      event.preventDefault();
      event.stopImmediatePropagation();
      const fields = { effect: button.dataset.effect || '' };
      if (button.dataset.vstIndex !== undefined) fields.index = Number(button.dataset.vstIndex);
      act(action, fields);
      return;
    }
    if (!current && (button.id === 'settings-open' || button.id === 'wood-layers')) {
      event.preventDefault();
      event.stopImmediatePropagation();
      return;
    }
    if (button.id === 'settings-open') {
      updateDialogs(current);
      selectSettingsTab('audio');
      document.querySelector('#settings-dialog').showModal();
      act('audio-open');
    }
    else if (button.hasAttribute('data-close-dialog')) document.querySelector('#settings-dialog').close();
    else if (button.dataset.settingsTab) selectSettingsTab(button.dataset.settingsTab);
    else if (button.id === 'demo-cables-toggle') {
      showCables = !showCables;
      localStorage.setItem('looper.showCables', String(showCables));
      updateVisualSettings();
      if (current) { renderStructure(current); updateDynamic(current); }
    }
    else if (button.dataset.studyPage) {
      visualPage = button.dataset.studyPage;
      updateVisualSettings();
      if (current) { renderStructure(current); updateDynamic(current); }
    }
    else if (button.dataset.nativeExpand) {
      expandedPedal = button.dataset.nativeExpand;
      const item = expandedPedal === 'looper' ? { id: 'looper', type: 'looper' }
        : current.effects.find(effect => effect.id === expandedPedal);
      if (item) {
        const index = expandedPedal === 'looper' ? current.deckIndex
          : current.effects.indexOf(item) + (current.effects.indexOf(item) >= current.deckIndex ? 1 : 0);
        renderExpanded(item, index);
        updateDynamic(current);
        document.querySelector('#study-pedal-dialog').showModal();
      }
    }
    else if (button.hasAttribute('data-study-close')) {
      document.querySelector('#study-pedal-dialog').close();
    }
    else if (button.id === 'tempo-tap') {
      const now = performance.now();
      if (lastTempoTap && now - lastTempoTap < 2000)
        act('setting', { name: 'bpm', value: Math.max(30, Math.min(300, Math.round(60000 / (now - lastTempoTap)))) });
      lastTempoTap = now;
    }
    else if (button.id === 'save-board-preset') {
      document.querySelector('#preset-name').value = current?.boardPreset || '';
      const dialog = document.querySelector('#preset-dialog');
      dialog.returnValue = '';
      dialog.showModal();
      document.querySelector('#preset-name').focus();
    }
    else if (button.id === 'layered-setting' || button.id === 'click-setting') {
      const name = button.id === 'layered-setting' ? 'layered' : 'metronome';
      act('setting', { name, value: Number(!current[name]) });
    }
    else if (button.dataset.vstTab) selectVstView(button.dataset.vstTab);
    else if (button.dataset.vstManufacturer !== undefined) {
      const manufacturer = button.dataset.vstManufacturer;
      if (expandedVstFolders.has(manufacturer)) expandedVstFolders.delete(manufacturer);
      else expandedVstFolders.add(manufacturer);
      renderVstPickerList();
    }
    else if (button.dataset.nativeCategory) {
      libraryCategory = libraryCategory === button.dataset.nativeCategory ? '' : button.dataset.nativeCategory;
      renderNativeLibrary(current?.pedalTemplates);
      document.querySelectorAll('.library-folder').forEach(folder => {
        if (folder.dataset.nativeCategory === libraryCategory) folder.focus();
      });
    }
    else if (button.id === 'wood-toolbox') setLibraryOpen(!document.body.classList.contains('toolbox-open'), true);
    else if (button.id === 'close-effects') setLibraryOpen(false, true);
    else if (button.id === 'wood-layers') {
      setLibraryOpen(false);
      updateDialogs(current);
      document.querySelector('#layers-dialog').showModal();
    }
    else if (button.dataset.audioAction) act('audio', { name: button.dataset.audioAction });
    else if (button.dataset.nativeLayerAction) {
      act('layer', { id: Number(button.closest('[data-native-layer]').dataset.nativeLayer), name: button.dataset.nativeLayerAction });
    }
    else if (button.id === 'wood-tuner') act('tuner');
    else if (button.dataset.boardSwitch) act('switch', { name: button.dataset.boardSwitch });

    else return;
    event.preventDefault();
    event.stopImmediatePropagation();
  }, true);

  document.querySelector('#effect-search').addEventListener('input', () => renderNativeLibrary(current?.pedalTemplates));
  document.querySelector('#preset-dialog').addEventListener('close', event => {
    if (event.target.returnValue === 'save')
      act('save-board', { name: document.querySelector('#preset-name').value });
  });
  document.querySelector('#board-preset').addEventListener('change', event => {
    const value = event.target.value;
    if (value) act('load-board', { name: value === 'reload' ? current?.boardPreset : value.slice(7) });
    else event.target.value = current?.boardPreset ? `preset:${current.boardPreset}` : '';
  });
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape' && document.querySelector('#vst-picker').open) {
      event.preventDefault();
      act('cancel-vst-picker');
      return;
    }
    if (event.key === 'Escape' && document.body.classList.contains('toolbox-open')) {
      event.preventDefault();
      setLibraryOpen(false, true);
    }
  });
  document.querySelector('#vst-picker').addEventListener('cancel', event => {
    event.preventDefault();
    act('cancel-vst-picker');
  });
  document.querySelector('#effect-library').addEventListener('dragstart', event => {
    const pedal = event.target.closest('[data-native-template]');
    if (!pedal) return;
    event.dataTransfer.setData('application/x-looper-template', pedal.dataset.nativeTemplate);
    event.dataTransfer.effectAllowed = 'copy';
    document.querySelector('#board').classList.add('accepting-drop');
  });
  document.querySelector('#effect-library').addEventListener('dragend', () => {
    document.querySelector('#board').classList.remove('accepting-drop', 'drag-over');
  });
  document.querySelector('#board').addEventListener('dragover', event => {
    if (!Array.from(event.dataTransfer.types).includes('application/x-looper-template')) return;
    event.preventDefault();
    event.dataTransfer.dropEffect = 'copy';
    document.querySelector('#board').classList.add('drag-over');
  });
  document.querySelector('#board').addEventListener('dragleave', event => {
    if (!document.querySelector('#board').contains(event.relatedTarget))
      document.querySelector('#board').classList.remove('drag-over');
  });
  document.querySelector('#board').addEventListener('drop', event => {
    const template = event.dataTransfer.getData('application/x-looper-template');
    if (!current?.pedalTemplates.some(item => item.id === template)) return;
    event.preventDefault();
    document.querySelector('#board').classList.remove('accepting-drop', 'drag-over');
    const cards = [...document.querySelectorAll('#pedals [data-pedal]')];
    const slot = visibleSlotAtPoint(cards, event.clientX, event.clientY);
    act('add-template', { template, slot: slotForVisibleCards(slot) });
  });

  document.addEventListener('input', event => {
    const input = event.target;
    if (input.dataset.volume !== undefined) {
      act('layer', { id: Number(input.dataset.volume), name: 'volume', value: Number(input.value) });
      input.closest('label').querySelector('output').textContent = `${input.value}%`;
      event.stopImmediatePropagation();
    } else if (input.dataset.pan !== undefined) {
      act('layer', { id: Number(input.dataset.pan), name: 'pan', value: Number(input.value) });
      const value = Number(input.value);
      input.closest('label').querySelector('output').textContent = value === 0 ? 'C' : `${value < 0 ? 'L' : 'R'}${Math.abs(value)}`;
      event.stopImmediatePropagation();
    }
  }, true);

  document.addEventListener('change', event => {
    const input = event.target;
    if (input.dataset.nativeSetting) {
      event.stopImmediatePropagation();
      if (!input.checkValidity()) { input.reportValidity(); return; }
      act('setting', { name: input.dataset.nativeSetting, value: Number(input.value) });
      return;
    }
    if (input.dataset.mapKnob !== undefined) {
      act('map-parameter', { effect: input.dataset.effect, knob: Number(input.dataset.mapKnob), parameter: input.value });
      event.stopImmediatePropagation();
      return;
    }
    if (input.dataset.audioSetting) {
      event.stopImmediatePropagation();
      const name = input.dataset.audioSetting;
      const value = input.type === 'checkbox' ? Number(input.checked) : Number(input.value);
      act('audio', { name, value, id: Number(input.dataset.index), text: input.type === 'checkbox' ? input.dataset.id : input.value });
      return;
    }
    const names = { 'bpm-setting': 'bpm', 'beats-setting': 'beats', 'unit-setting': 'unit',
      'count-setting': 'countIn', 'loop-bars-setting': 'loopBars', 'native-input-mode': 'inputMode' };
    if (!Object.hasOwn(names, input.id)) return;
    event.stopImmediatePropagation();
    if (!input.checkValidity()) { input.reportValidity(); return; }
    act('setting', { name: names[input.id], value: input.type === 'checkbox' ? Number(input.checked) : Number(input.value) });
  }, true);

  document.querySelector('#settings-dialog').addEventListener('close', () => act('audio-close'));
  document.querySelector('#study-pedal-dialog').addEventListener('close', () => {
    const id = expandedPedal;
    expandedPedal = null;
    document.querySelector('#study-expanded-pedal').replaceChildren();
    if (id) [...document.querySelectorAll('[data-native-expand]')].find(button => button.dataset.nativeExpand === id)?.focus();
  });
  document.querySelector('#visual-view').addEventListener('change', event => {
    const view = { default: 'default', 'board-1': '01', 'board-2': '02', 'board-3': '03' }[event.target.value];
    if (!view) {
      updateVisualSettings();
      return;
    }
    visualView = view;
    visualPage = 'loop';
    localStorage.setItem('looper.visualView', visualView);
    updateVisualSettings();
    if (current) { renderStructure(current); updateDynamic(current); }
  });

  document.addEventListener('dblclick', event => {
    const input = event.target.closest('input[data-pan]');
    if (!input) return;
    input.value = 0;
    input.dispatchEvent(new Event('input', { bubbles: true }));
  }, true);

  document.addEventListener('pointerdown', event => {
    const control = event.target.closest('.level-dial');
    if (control && control.getAttribute('aria-disabled') !== 'true' && event.button === 0) {
      drag = { control, pointer: event.pointerId, y: event.clientY, value: Number(control.getAttribute('aria-valuenow')) };
      control.focus();
      control.setPointerCapture(event.pointerId);
      event.preventDefault();
      event.stopImmediatePropagation();
      return;
    }
    const handle = event.target.closest('[data-drag]');
    if (handle && event.button === 0 && current?.effects.length) {
      const card = handle.closest('[data-pedal]');
      drag = { handle, card, pointer: event.pointerId, x: event.clientX, y: event.clientY, startX: event.clientX, startY: event.clientY };
      handle.setPointerCapture(event.pointerId);
      event.preventDefault();
      event.stopImmediatePropagation();
    }
  }, true);

  document.addEventListener('pointermove', event => {
    if (!drag || drag.pointer !== event.pointerId) return;
    if (drag.control) requestDial(drag.control, drag.value + (drag.y - event.clientY) * (drag.control.dataset.parameterKnob !== undefined ? 5 : .5));
    else {
      drag.x = event.clientX;
      drag.y = event.clientY;
      if (Math.hypot(drag.x - drag.startX, drag.y - drag.startY) > 5) drag.card.classList.add('reordering');
      drag.card.style.translate = `${drag.x - drag.startX}px ${drag.y - drag.startY}px`;
      const bounds = document.querySelector('#board').getBoundingClientRect();
      const viewport = document.querySelector('.board-viewport').getBoundingClientRect();
      const left = Math.max(bounds.left, viewport.left, 0);
      const right = Math.min(bounds.right, viewport.right, innerWidth);
      const transferPage = visualView === '03' && (visualPage === 'pre' && event.clientX >= right - 65
        ? 'post' : visualPage === 'post' && event.clientX <= left + 65 ? 'pre' : '');
      const edge = document.querySelector('[data-study-edge]');
      edge.hidden = !transferPage;
      if (transferPage) {
        edge.style.left = `${(transferPage === 'post' ? right - 65 : left) - bounds.left}px`;
        edge.textContent = `DROP TO ${transferPage.toUpperCase()}`;
      }
    }
    event.preventDefault();
    event.stopImmediatePropagation();
  }, true);

  function finishPointer(event) {
    if (!drag || drag.pointer !== event.pointerId) return;
    if (drag.control) {
      if (drag.control.hasPointerCapture(event.pointerId)) drag.control.releasePointerCapture(event.pointerId);
      drag = null;
      event.stopImmediatePropagation();
      return;
    }
    const { card } = drag;
    const otherCards = [...document.querySelectorAll('#pedals [data-pedal]')].filter(item => item !== card);
    const slot = slotForVisibleCards(visibleSlotAtPoint(otherCards, event.clientX, event.clientY));
    const id = card.dataset.pedal;
    const changed = card.classList.contains('reordering');
    const board = document.querySelector('#board').getBoundingClientRect();
    const viewport = document.querySelector('.board-viewport').getBoundingClientRect();
    const left = Math.max(board.left, viewport.left, 0);
    const right = Math.min(board.right, viewport.right, innerWidth);
    const inside = event.clientX >= left && event.clientX <= right
      && event.clientY >= Math.max(board.top, viewport.top, 0)
      && event.clientY <= Math.min(board.bottom, viewport.bottom, innerHeight);
    const transferPage = visualView === '03' && inside
      ? visualPage === 'pre' && event.clientX >= right - 65 ? 'post'
        : visualPage === 'post' && event.clientX <= left + 65 ? 'pre' : ''
      : '';
    document.querySelector('[data-study-edge]').hidden = true;
    card.style.translate = '';
    card.classList.remove('reordering');
    const handle = drag.handle;
    drag = null;
    if (handle.hasPointerCapture(event.pointerId)) handle.releasePointerCapture(event.pointerId);
    if (changed && event.type === 'pointerup' && inside) {
      if (transferPage) {
        visualPage = transferPage;
        updateVisualSettings();
        act('reorder', { id, slot: current.deckIndex });
      } else act('reorder', { id, slot });
    }
    event.stopImmediatePropagation();
  }
  document.addEventListener('pointerup', finishPointer, true);
  document.addEventListener('pointercancel', finishPointer, true);
  document.addEventListener('lostpointercapture', finishPointer, true);

  document.addEventListener('keydown', event => {
    const handle = event.target.closest('[data-drag], [data-native-expand]');
    if (handle && (event.key === 'ArrowLeft' || event.key === 'ArrowRight')) {
      event.preventDefault();
      event.stopImmediatePropagation();
      const id = handle.dataset.drag || handle.dataset.nativeExpand;
      const effectIndex = current.effects.findIndex(effect => effect.id === id);
      const source = id === 'looper' ? current.deckIndex
        : effectIndex + (effectIndex >= current.deckIndex ? 1 : 0);
      if (visualView === '03' && ((visualPage === 'pre' && event.key === 'ArrowRight' && source === current.deckIndex - 1)
        || (visualPage === 'post' && event.key === 'ArrowLeft' && source === current.deckIndex + 1))) {
        visualPage = visualPage === 'pre' ? 'post' : 'pre';
      }
      const delta = event.key === 'ArrowLeft' ? -1 : 1;
      if (source + delta >= 0 && source + delta <= current.effects.length) pendingFocus = id;
      act('move', { id, delta });
      return;
    }
    const dialControl = event.target.closest('.level-dial');
    if (!dialControl || !current || dialControl.getAttribute('aria-disabled') === 'true') return;
    const min = Number(dialControl.getAttribute('aria-valuemin'));
    const max = Number(dialControl.getAttribute('aria-valuemax'));
    const value = Number(dialControl.getAttribute('aria-valuenow'));
    const step = dialControl.dataset.parameterKnob !== undefined ? (event.shiftKey ? 50 : 10) : (event.shiftKey ? 5 : 1);
    const changes = { ArrowUp: value + step, ArrowRight: value + step, ArrowDown: value - step,
      ArrowLeft: value - step, PageUp: value + step * 10, PageDown: value - step * 10, Home: min, End: max };
    if (Object.hasOwn(changes, event.key)) {
      event.preventDefault();
      event.stopImmediatePropagation();
      requestDial(dialControl, changes[event.key]);
    }
  }, true);

  let viewportWidth = 0;
  new ResizeObserver(() => {
    const width = document.querySelector('.board-viewport').clientWidth;
    if (width === viewportWidth || !current || drag) return;
    viewportWidth = width;
    const allPedals = [...current.effects];
    allPedals.splice(current.deckIndex, 0, { id: 'looper', type: 'looper' });
    const compact = visualView !== 'default' && !(visualView === '03' && visualPage === 'loop');
    layoutNativeBoard(allPedals, compact);
  }).observe(document.querySelector('.board-viewport'));
})();
