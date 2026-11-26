'use strict';

document.addEventListener('pointerdown', () => document.documentElement.classList.add('pointer-input'), true);
document.addEventListener('keydown', event => {
  if (['Tab', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key))
    document.documentElement.classList.remove('pointer-input');
}, true);

const $ = (selector) => document.querySelector(selector);
const concepts = {
  wood: { number: 'THE VINTAGE WORKBENCH', title: 'Old-school feel. New ideas.',
    description: 'A wood-backed board, dark metal rails and a leather-trimmed loop deck. Physical controls, tape-style reels, and a cream mixing strip.',
    note: 'Reference-inspired, original artwork<br><strong>A looper, not an amplifier</strong>', workspace: 'VINTAGE WORKBENCH',
    board: 'The loop deck', hint: '' }
};
const effects = {
  compressor: { name: 'COMPRESSOR', category: 'Dynamics', color: '#82a2c8', knobs: ['Blend', 'Compression', 'Tone', 'Volume'] },
  gate: { name: 'NOISE GATE', category: 'Dynamics', color: '#a6b2b1', knobs: ['Threshold', 'Release', 'Hold', 'Level'] },
  limiter: { name: 'LIMITER', category: 'Dynamics', color: '#cdad7c', knobs: ['Threshold', 'Release', 'Ceiling'] },
  grit: { name: 'OVERDRIVE', category: 'Gain', color: '#d5bc77', knobs: ['Drive', 'Tone', 'Level'] },
  distortion: { name: 'DISTORTION', category: 'Gain', color: '#c58f98', knobs: ['Gain', 'Tone', 'Level'] },
  fuzz: { name: 'FUZZ', category: 'Gain', color: '#b49bcd', knobs: ['Fuzz', 'Tone', 'Volume'] },
  boost: { name: 'BOOST', category: 'Gain', color: '#cdad7c', knobs: ['Boost', 'Tone', 'Level'] },
  wah: { name: 'WAH', category: 'Filter', color: '#c5ca86', knobs: ['Position', 'Range', 'Level'] },
  envelope: { name: 'ENVELOPE FILTER', category: 'Filter', color: '#91b9b3', knobs: ['Sensitivity', 'Filter', 'Mix'] },
  eq: { name: 'EQUALIZER', category: 'Filter', color: '#82a2c8', knobs: ['Bass', 'Mid', 'Treble', 'Level'] },
  octave: { name: 'OCTAVER', category: 'Pitch', color: '#b49bcd', knobs: ['Octave', 'Blend', 'Direct', 'Level'] },
  pitch: { name: 'PITCH SHIFT', category: 'Pitch', color: '#c58f98', knobs: ['Shift', 'Fine', 'Blend', 'Level'] },
  harmonizer: { name: 'HARMONIZER', category: 'Pitch', color: '#a6b2b1', knobs: ['Key', 'Interval', 'Mix', 'Level'] },
  chorus: { name: 'CHORUS', category: 'Modulation', color: '#91b9b3', knobs: ['Rate', 'Depth', 'Tone', 'Mix'] },
  flanger: { name: 'FLANGER', category: 'Modulation', color: '#82a2c8', knobs: ['Rate', 'Depth', 'Feedback', 'Mix'] },
  phaser: { name: 'PHASER', category: 'Modulation', color: '#b49bcd', knobs: ['Rate', 'Depth', 'Feedback', 'Mix'] },
  tremolo: { name: 'TREMOLO', category: 'Modulation', color: '#cdad7c', knobs: ['Rate', 'Depth', 'Shape', 'Level'] },
  tide: { name: 'DIGITAL DELAY', category: 'Delay', color: '#91b9b3', knobs: ['Time', 'Feedback', 'Mix', 'Tone'] },
  tape: { name: 'TAPE ECHO', category: 'Delay', color: '#a6b2b1', knobs: ['Time', 'Age', 'Feedback', 'Mix'] },
  halo: { name: 'ROOM REVERB', category: 'Reverb', color: '#c2c8a9', knobs: ['Decay', 'Tone', 'Mix'] },
  spring: { name: 'SPRING REVERB', category: 'Reverb', color: '#d5bc77', knobs: ['Dwell', 'Tone', 'Mix'] },
  shimmer: { name: 'SHIMMER', category: 'Reverb', color: '#b49bcd', knobs: ['Decay', 'Shimmer', 'Mix'] },
  amp: { name: 'AMP SIM', category: 'Amp & Cab', color: '#c58f98', knobs: ['Gain', 'Bass', 'Treble', 'Level'] },
  cab: { name: 'CAB SIM', category: 'Amp & Cab', color: '#cdad7c', knobs: ['Mic', 'Room', 'Air', 'Level'] }
};
const effectCategories = ['Dynamics', 'Gain', 'Filter', 'Pitch', 'Modulation', 'Delay', 'Reverb', 'Amp & Cab'];
const palettes = ['#da9e79', '#8bb8b0', '#c5ca86', '#b49bcd', '#82a2c8', '#cdad7c', '#c58f98', '#a6b2b1'];
const studyMinPedalHeight = 1;
const footerControls = {
  input: { min: -24, max: 24, unit: 'dB', value: 0 },
  gate: { min: -80, max: 0, unit: 'dB', value: -50, switchable: true },
  transpose: { min: -12, max: 12, unit: 'st', value: 0 },
  doubler: { min: 0, max: 50, unit: 'ms', value: 15, switchable: true },
  output: { min: -24, max: 24, unit: 'dB', value: 0 }
};
let nextId = 10;
const states = {};
const demoPresets = Object.create(null);
let concept = 'wood';
let state;
let study = '';
let studyPage = 'loop';
let expandedPedal = null;
let toastTimer;
let lastTempoTap = 0;
let confirmation;
let drag;
let dialDrag;
let reorderDrag;
let studyDrag;
let studyPointer;
let demoCablesEnabled = false;
let frameTime = performance.now();
const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;

function freshState(mode) {
  const previewChain = ['compressor', 'grit', 'eq', 'chorus', 'looper', 'tide', 'tape', 'halo', 'shimmer']
    .map((type, index) => type === 'looper' ? { id: 'looper', type } : { id: `${type}-${index}`, type, active: true });
  return { playing: true, recording: false, layers: [
    { id: 1, name: 'Rhythm foundation', volume: 82, pan: 0, active: true, solo: false },
    { id: 2, name: 'A little atmosphere', volume: 64, pan: -24, active: true, solo: false },
    { id: 3, name: 'Lead idea', volume: 75, pan: 18, active: true, solo: false }
  ], chain: mode.startsWith('board-') ? previewChain : mode === 'wood' ? [{ id: 'looper', type: 'looper' }]
    : [{ id: 'grit-1', type: 'grit', active: true }, { id: 'looper', type: 'looper' }, { id: 'halo-1', type: 'halo', active: true }],
  levels: { input: 80, live: 70, loop: 85, output: 60 },
  footer: Object.fromEntries(Object.entries(footerControls).map(([name, control]) => [name, control.value])),
  gate: false, doubler: false, tuner: true, toolbox: false, effectCategory: 'Dynamics',
  positions: {}, bpm: 120, beats: 4, unit: 4, count: 0, autoStopBars: 0,
  metronome: true, layered: true, progress: .35, undo: null };
}

function notify(message) {
  $('#toast').textContent = message;
  $('#toast').classList.add('visible');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => $('#toast').classList.remove('visible'), 3500);
}

const galleryEmblems = {
  compressor: '<path d="M54 132 H76 M124 132 H146 M66 120 L78 132 L66 144 M134 120 L122 132 L134 144"/><rect x="85" y="118" width="30" height="28" rx="3"/>',
  gate: '<path d="M63 150 V116 H137 V150 M80 150 V124 M100 150 V124 M120 150 V124 M55 150 H145"/>',
  limiter: '<path d="M58 150 H142 M58 119 H142 M68 142 V127 M84 142 V131 M100 142 V124 M116 142 V134 M132 142 V129"/>',
  grit: '<path d="M54 143 L67 143 L74 123 L85 148 L95 119 L106 148 L117 123 L125 143 H146"/>',
  distortion: '<path d="M55 147 L71 118 L81 147 L95 118 L105 147 L119 118 L129 147 H145"/>',
  fuzz: '<path d="M57 147 L66 119 L75 147 L84 119 L93 147 L102 119 L111 147 L120 119 L129 147 L138 119"/>',
  boost: '<path d="M100 149 V118 M86 131 L100 117 L114 131 M65 150 H135"/>',
  wah: '<path d="M68 119 H132 M64 128 H136 M61 137 H139 M58 146 H142"/>',
  envelope: '<path d="M56 145 Q68 145 77 120 Q84 145 94 145 Q104 145 113 128 Q120 145 144 145"/>',
  eq: '<path d="M68 116 V150 M89 116 V150 M111 116 V150 M132 116 V150"/><path d="M62 128 H74 M83 140 H95 M105 123 H117 M126 134 H138"/>',
  octave: '<circle cx="82" cy="133" r="17"/><circle cx="118" cy="133" r="17"/><path d="M82 124 V142 M73 133 H91 M109 133 H127"/>',
  pitch: '<path d="M58 148 L85 121 L103 139 L131 116 M120 116 H131 V127 M72 151 H142"/>',
  harmonizer: '<path d="M56 149 L79 123 L100 143 L121 120 L144 144 M56 137 L79 111 L100 131 L121 108 L144 132"/>',
  chorus: '<path d="M54 130 Q66 110 78 130 T102 130 T126 130 T146 130 M54 143 Q66 123 78 143 T102 143 T126 143 T146 143"/>',
  flanger: '<path d="M55 146 Q76 109 100 133 T145 120 M55 120 Q76 157 100 133 T145 146"/>',
  phaser: '<circle cx="100" cy="133" r="19"/><circle cx="100" cy="133" r="11"/><circle cx="100" cy="133" r="3"/><path d="M57 133 H76 M124 133 H143"/>',
  tremolo: '<path d="M53 146 Q58 120 64 133 T75 133 T88 133 T104 133 T123 133 T147 133"/>',
  tide: '<path d="M55 146 H71 V120 H84 V146 H98 V126 H112 V146 H126 V132 H145"/>',
  tape: '<circle cx="78" cy="132" r="14"/><circle cx="122" cy="132" r="14"/><circle cx="78" cy="132" r="3"/><circle cx="122" cy="132" r="3"/><path d="M78 118 H122 M78 146 H122"/>',
  halo: '<path d="M57 150 Q100 96 143 150 M70 150 Q100 115 130 150 M83 150 Q100 133 117 150"/>',
  spring: '<path d="M54 134 H62 Q67 116 73 134 T84 134 T95 134 T106 134 T117 134 T128 134 T139 134 H146"/>',
  shimmer: '<path d="M100 112 L105 127 L120 132 L105 137 L100 152 L95 137 L80 132 L95 127 Z M65 118 V128 M60 123 H70 M135 139 V149 M130 144 H140"/>',
  amp: '<rect x="56" y="116" width="88" height="35" rx="3"/><path d="M63 125 H137 M66 137 H96"/><circle cx="123" cy="137" r="5"/>',
  cab: '<rect x="58" y="113" width="84" height="42" rx="3"/><path d="M62 121 L94 151 M79 117 L115 151 M98 117 L136 152 M117 117 L138 137"/>'
};

const galleryPalettes = {
  Dynamics: [210, 54, 60], Gain: [0, 0, 16], Filter: [28, 82, 60], Pitch: [52, 80, 60],
  Modulation: [130, 42, 60], Delay: [270, 42, 60], Reverb: [0, 60, 60], 'Amp & Cab': [25, 50, 30]
};
const galleryControls = {
  halo: ['Decay', 'Pre-delay', 'Size', 'Low', 'High', 'Mix'],
  spring: ['Dwell', 'Decay', 'Pre-delay', 'Low', 'High', 'Mix'],
  tide: ['Time', 'Feedback', 'Mix', 'Tone', 'Rate', 'Depth'],
  tape: ['Time', 'Feedback', 'Mix', 'Age', 'Wow', 'Flutter'],
  amp: ['Gain', 'Level'],
  cab: ['Mic', 'Level']
};

function galleryKnob(label, x, y, radius = 8) {
  return `<circle cx="${x}" cy="${y + 2}" r="${radius + 3}" fill="#171714" stroke="#e6d9c0" stroke-width="1.5"/>
    <circle cx="${x}" cy="${y}" r="${radius}" fill="#353530" stroke="#100f0e" stroke-width="1"/>
    <path d="M${x} ${y - radius + 1} V${y - 2}" stroke="#f3ead5" stroke-width="1.5"/>
    <text x="${x}" y="${y + radius + 11}" text-anchor="middle" fill="var(--gallery-ink)" font-size="5" font-weight="700">${label.toUpperCase()}</text>`;
}

function gallerySwitch(x, y, label) {
  return `<circle cx="${x}" cy="${y - 19}" r="2.5" fill="#f4d791" stroke="#554739"/>
    <circle cx="${x}" cy="${y}" r="12" fill="#77736a" stroke="#302e2a" stroke-width="3"/>
    <circle cx="${x}" cy="${y - 2}" r="8" fill="#d2cdc0" stroke="#f4ecda" stroke-width="2"/>
    <text x="${x}" y="${y + 19}" text-anchor="middle" fill="var(--gallery-ink)" font-size="5" font-weight="700">${label}</text>`;
}

function galleryJacks(left, right, y) {
  return [[left, -1], [right, 1]].map(([x, direction]) =>
    `<g class="gallery-jack" transform="translate(${x} ${y}) scale(${direction} 1)" aria-hidden="true">
      <path d="M0 -13 H7 L10 -10 V10 L7 13 H0 Z" fill="#222321" stroke="#111210" stroke-width="1"/>
      <path d="M2 -12 V12 M6 -10 V10" stroke="#62635d" stroke-width="1.5"/>
    </g>`).join('');
}

function renderGallery() {
  $('#gallery-collection').innerHTML = effectCategories.map((category, categoryIndex) => {
    const pedals = Object.entries(effects).filter(([, effect]) => effect.category === category);
    return `<section class="gallery-family" aria-labelledby="gallery-family-${categoryIndex}">
      <header class="gallery-family-header"><div><span class="eyebrow">FAMILY / 0${categoryIndex + 1}</span><h2 id="gallery-family-${categoryIndex}">${category}</h2></div><span>${String(pedals.length).padStart(2, '0')} DESIGNS</span></header>
      <div class="gallery-grid">${pedals.map(([type, effect], index) => {
        const controls = galleryControls[type] || effect.knobs;
        const expanded = controls.length > 3 || ['Delay', 'Amp & Cab'].includes(category) || ['halo', 'spring'].includes(type);
        return `<article class="gallery-card">
          <div class="gallery-art">${galleryPedal(type)}</div>
          <div class="gallery-card-caption"><div><h3>${effect.name}</h3><span>${expanded ? 'LARGER ENCLOSURE' : 'COMPACT ENCLOSURE'} · ${controls.length} DIALS</span><br><span>${controls.join(' / ')}</span></div><span class="gallery-card-index">0${categoryIndex + 1}.${String(index + 1).padStart(2, '0')}</span></div>
        </article>`;
      }).join('')}</div>
    </section>`;
  }).join('');
  document.querySelectorAll('#pedal-gallery .gallery-art svg').forEach(svg => {
    sizeGalleryArt(svg);
    svg.closest('.gallery-card').style.width = `${Math.max(210, parseFloat(svg.style.width) + 56)}px`;
  });
}

function sizeGalleryArt(svg, pedalHeight = 200) {
  const body = svg.querySelector('[data-gallery-body]');
  const x = Number(body.getAttribute('x')), y = Number(body.getAttribute('y'));
  const width = Number(body.getAttribute('width')), height = Number(body.getAttribute('height'));
  svg.setAttribute('viewBox', `${x - 12} ${y - 12} ${width + 24} ${height + 24}`);
  svg.style.width = `${pedalHeight * (width + 24) / height}px`;
  svg.style.height = `${pedalHeight * (height + 24) / height}px`;
}

function galleryAppearance(type) {
  const category = effects[type].category;
  const index = Object.values(effects).filter(effect => effect.category === category).indexOf(effects[type]);
  const [hue, saturation, baseLightness] = galleryPalettes[category];
  const lightness = category === 'Gain' ? [16, 36, 62, 92][index] : baseLightness + index * 5;
  return {
    color: `hsl(${hue} ${saturation}% ${lightness}%)`,
    ink: category === 'Amp & Cab' || (category === 'Gain' && index < 2) ? '#f4ede0' : '#242521',
    index
  };
}

function galleryPedal(type) {
        const effect = effects[type];
        const category = effect.category;
        const { color, ink, index } = galleryAppearance(type);
        const controls = galleryControls[type] || effect.knobs;
        const layout = type === 'halo' || type === 'spring' ? 'reverb'
          : category === 'Delay' ? 'delay' : category === 'Amp & Cab' ? 'amp-cab' : 'stompbox';
        const expanded = controls.length > 3 || layout !== 'stompbox';
        const wide = ['eq', 'harmonizer'].includes(type);
        const width = wide ? 164 : expanded ? 142 : type === 'boost' ? 86 : 110;
        const left = (200 - width) / 2;
        const knobs = controls.map((label, knob) => {
          const x = !expanded && knob === 2 ? 100 : 100 + (knob % 2 ? 24 : -24);
          const y = knob < 2 ? 54 : 90;
          return galleryKnob(label, x, y);
        }).join('');
        const largeKnobs = controls.map((label, knob) => {
          if (layout === 'amp-cab') return galleryKnob(label, knob ? 161 : 39, 82, 11);
          if (layout === 'reverb' && knob >= 4) return galleryKnob(label, knob === 4 ? 84 : 116, 82, 7);
          const x = knob % 2 ? 157 : 43;
          const y = (layout === 'delay' ? 68 : 67) + Math.floor(knob / 2) * (layout === 'delay' ? 32 : 36);
          return galleryKnob(label, x, y, 7);
        }).join('');
        const largeArt = `${galleryJacks(11, 189, 135)}<rect data-gallery-body x="11" y="${layout === 'delay' ? 44 : 27}" width="178" height="${layout === 'delay' ? 164 : 198}" rx="${layout === 'reverb' ? 13 : 7}" fill="${color}" stroke="#25231f" stroke-width="4"/>
          <rect x="15" y="${layout === 'delay' ? 48 : 31}" width="170" height="${layout === 'delay' ? 156 : 190}" rx="${layout === 'reverb' ? 10 : 5}" fill="url(#finish-${type})" stroke="#fff" stroke-opacity=".3"/>
          <text x="100" y="${layout === 'delay' ? 57 : 42}" text-anchor="middle" fill="var(--gallery-ink)" font-size="6" font-weight="800" letter-spacing="1.5">LOOPER / ${effect.category.toUpperCase()}</text>
          ${largeKnobs}
          ${layout === 'delay' ? `<rect x="71" y="66" width="58" height="29" rx="3" fill="#363839" stroke="#c8c0b4"/>
            <text x="100" y="78" text-anchor="middle" fill="#dfebe4" font-size="5">${type === 'tape' ? 'TAPE ECHO' : 'DIGITAL'}</text>
            <text x="100" y="88" text-anchor="middle" fill="#dfebe4" font-size="6">TIME / FEEDBACK</text>` : ''}
          ${layout === 'amp-cab' ? `<rect x="68" y="55" width="64" height="60" rx="5" fill="#242b2b" stroke="#b6ae9f" stroke-width="2"/>
            <text x="100" y="75" text-anchor="middle" fill="#d2e8dd" font-size="9" font-weight="700">${type === 'amp' ? 'AMP' : 'CAB'}</text>
            <path d="M78 94 H122 M78 100 H112" stroke="#95c7b5" stroke-width="2"/>
            <g fill="#e1ddd2" stroke="#595650"><rect x="49" y="130" width="28" height="12" rx="2"/><rect x="86" y="130" width="28" height="12" rx="2"/><rect x="123" y="130" width="28" height="12" rx="2"/></g>` : ''}
          <text class="gallery-pedal-name" x="100" y="${layout === 'amp-cab' ? 158 : layout === 'delay' ? 154 : 160}" text-anchor="middle" fill="var(--gallery-ink)" font-size="8" font-weight="800">${effect.name}</text>
          ${layout === 'delay' || layout === 'reverb' ? `<g class="gallery-emblem" transform="translate(100 120) scale(.65) translate(-100 -132)" fill="none" stroke="var(--gallery-ink)" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">${galleryEmblems[type]}</g>` : ''}
          ${gallerySwitch(51, layout === 'delay' ? 179 : 192, 'BYPASS')}
          ${gallerySwitch(149, layout === 'delay' ? 179 : 192, 'MODE')}`;
        return `<svg viewBox="0 0 200 250" role="img" aria-label="${effect.name} pedal concept" data-layout="${layout}" data-color="${color}" style="--gallery-ink:${ink}">
            <defs><linearGradient id="finish-${type}" x1="0" x2="1" y1="0" y2="1">
              <stop stop-color="#fff" stop-opacity=".28"/><stop offset=".45" stop-color="#fff" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity=".28"/>
            </linearGradient></defs>
            ${layout !== 'stompbox' ? largeArt : `
            ${galleryJacks(left, left + width, 137)}
            <rect data-gallery-body x="${left}" y="17" width="${width}" height="218" rx="${wide ? 7 : 12}" fill="${color}" stroke="#25231f" stroke-width="4"/>
            <rect x="${left + 3}" y="20" width="${width - 6}" height="212" rx="${wide ? 5 : 10}" fill="url(#finish-${type})" stroke="#fff" stroke-opacity=".28"/>
            <circle cx="${left + 9}" cy="26" r="2" fill="#494943"/><circle cx="${left + width - 9}" cy="26" r="2" fill="#494943"/>
            <text x="100" y="32" text-anchor="middle" fill="var(--gallery-ink)" font-size="6" font-weight="800" letter-spacing="1.5">LOOPER / 0${index + 1}</text>
            ${knobs}
            <g transform="translate(100 0) scale(${expanded ? .85 : .65} .75) translate(-100 35)" fill="none" stroke="var(--gallery-ink)" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">${galleryEmblems[type]}</g>
            <text class="gallery-pedal-name" x="100" y="166" text-anchor="middle" fill="var(--gallery-ink)" font-size="8" font-weight="800" ${effect.name.length > 12 ? `textLength="${width - 20}" lengthAdjust="spacingAndGlyphs"` : ''}>${effect.name}</text>
            <circle cx="100" cy="180" r="3" fill="#f4d791" stroke="#554739" stroke-width="1"/>
            <circle cx="100" cy="207" r="14" fill="#77736a" stroke="#302e2a" stroke-width="3"/>
            <circle cx="100" cy="205" r="9" fill="#d2cdc0" stroke="#f4ecda" stroke-width="2"/>`}
          </svg>`;
}

function setConcept() {
  if (studyPointer) finishStudyPointer({ pointerId: studyPointer.pointer, type: 'pointercancel' });
  const gallery = location.hash === '#gallery' && !window.__JUCE__?.backend;
  study = !window.__JUCE__?.backend && ['#board-1', '#board-2', '#board-3'].includes(location.hash) ? location.hash.slice(1) : '';
  concept = 'wood';
  state = states[study || concept] ??= freshState(study || concept);
  $('#visual-view').value = study || 'board-2';
  expandedPedal = null;
  if ($('#study-pedal-dialog').open) $('#study-pedal-dialog').close();
  document.body.dataset.study = study;
  $('#study-pages').hidden = study !== 'board-3';
  $('.board-toolbar').prepend($('#wood-tuner'));
  $('#study-tuner-cable').toggleAttribute('hidden', !study);
  $('#pedal-gallery').hidden = !gallery;
  $('.concept-intro').hidden = gallery;
  $('.app-shell').hidden = gallery;
  $('.demo-explainer').hidden = gallery;
  if (gallery) renderGallery();
  document.body.dataset.concept = concept;
  const wood = concept === 'wood';
  setToolbox(wood && state.toolbox);
  for (const id of ['#wood-toolbox', '#wood-layers', '#wood-tuner', '#wood-footer', '#close-effects']) $(id).hidden = !wood;
  $('.level-strip:not(#wood-footer)').hidden = wood;
  $('#layout-reset').hidden = wood;
  $('#layers-dialog').close();
  if (wood) $('#layers-dialog').append($('#layer-mixer'));
  else $('.chain-strip').after($('#layer-mixer'));
  $('#board').style.minWidth = '';
  const design = study ? {
    ...concepts.wood,
    number: `PEDALBOARD STUDY / 0${study.slice(-1)}`,
    title: { 'board-1': 'The fixed loop deck.', 'board-2': 'The movable loop deck.', 'board-3': 'One signal chain. Three views.' }[study],
    description: { 'board-1': 'A fixed loop deck between adaptive rows of compact pre and post effects. Select a pedal to see its full controls.',
      'board-2': 'The loop deck joins the pedal chain. Reorder it with the effects and edit its controls from the footer.',
      'board-3': 'Switch between pre effects, the loop deck and post effects using the page buttons above the board.' }[study]
  } : concepts[concept];
  $('#concept-number').textContent = design.number;
  $('#concept-title').textContent = design.title;
  $('#concept-description').textContent = design.description;
  $('#concept-note').innerHTML = design.note;
  $('#workspace-name').textContent = design.workspace;
  $('#board-title').textContent = design.board;
  $('#board-hint').textContent = design.hint;
  updateDemoPresets();
  document.querySelectorAll('[data-concept-link]').forEach(link => {
    if (link.dataset.conceptLink === (gallery ? 'gallery' : study || concept)) link.setAttribute('aria-current', 'page');
    else link.removeAttribute('aria-current');
  });
  render();
}

function setToolbox(open, restoreFocus = false) {
  if (concept === 'wood') state.toolbox = open;
  document.body.classList.toggle('toolbox-open', open);
  $('#wood-toolbox').setAttribute('aria-expanded', open);
  if (restoreFocus) $('#wood-toolbox').focus();
}

function pedalHandle(id, label) {
  if (study) return '';
  return `data-drag="${id}"${concept === 'wood'
    ? ` tabindex="0" aria-label="Reorder ${label}. Use Left or Right to move." title="Drag to reorder the signal chain"`
    : ` title="${concept === 'canvas' ? 'Drag to position; use arrows to change order' : 'Use arrows below to change order'}"`}`;
}

function moveTools(id, label, index, removable = false) {
  const arrows = concept === 'wood' ? '' : `<button data-move="${id}" data-direction="-1" aria-label="Move ${label} earlier" ${index === 0 ? 'disabled' : ''}>←</button><button data-move="${id}" data-direction="1" aria-label="Move ${label} later" ${index === state.chain.length - 1 ? 'disabled' : ''}>→</button>`;
  const remove = removable ? `<button data-remove-pedal="${id}" aria-label="Remove demo ${label}">×</button>` : '';
  return arrows || remove ? `<div class="move-tools">${arrows}${remove}</div>` : '';
}

function renderLibrary() {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const query = $('#effect-search').value.toLowerCase();
  const categories = effectCategories.map(category => {
    const pedals = Object.entries(effects).filter(([, effect]) => effect.category === category
      && (!query || `${effect.name} ${effect.category}`.toLowerCase().includes(query)));
    if (query && !pedals.length) return '';
    const id = category.toLowerCase().replace(/[^a-z0-9]+/g, '-');
    const expanded = query ? true : state.effectCategory === category;
    return `<section class="library-category">
      <button class="library-folder" data-category="${category}" aria-expanded="${expanded}" aria-controls="folder-${id}">
        <span aria-hidden="true">${expanded ? '▾' : '▸'}</span>${category}<small>${pedals.length}</small>
      </button>
      <div id="folder-${id}" class="library-pedals" ${expanded ? '' : 'hidden'}>
        ${pedals.map(([type, effect]) => `<div class="library-pedal" draggable="true" data-effect-drag="${type}" title="Drag ${effect.name} onto the pedalboard">
          <span class="effect-swatch" style="background:${effectColor(type)}" aria-hidden="true"></span>
          <span class="library-pedal-name"><strong>${effect.name}</strong><small>${effect.knobs.join(' · ')}</small></span>
          <button data-add-type="${type}" aria-label="Add ${effect.name} to the pedalboard">+</button>
        </div>`).join('')}
      </div>
    </section>`;
  }).join('');
  $('#effect-library').innerHTML = categories || '<p class="library-empty">No matching pedals.</p>';
}

function effectColor(type) {
  return galleryAppearance(type).color;
}

function effectMarkup(pedal, index) {
  const effect = effects[pedal.type];
  const side = index < state.chain.findIndex(item => item.type === 'looper') ? 'PRE-LOOP' : 'POST-LOOP';
  return `<article class="pedal effect ${pedal.active ? '' : 'off'}" data-pedal="${pedal.id}" style="--pedal:${effectColor(pedal.type)};--pedal-ink:${galleryAppearance(pedal.type).ink}">
    <span class="pedal-jacks" aria-hidden="true"></span>
    <div class="drag-handle" ${pedalHandle(pedal.id, effect.name)}>
    <div class="pedal-number"><span>0${index + 1} / ${side}</span><span>DEMO</span></div>
    <h3 class="pedal-title">${effect.name}</h3><span class="pedal-category">${effect.category}</span></div>
    ${concept === 'wood' ? `<div class="effect-panel ${pedal.type}"><span>${effect.category}</span><small>STEREO EFFECT / DEMO</small><div class="vst-management"><button data-vst-add="${pedal.id}">ADD VST3</button></div></div>` : ''}
    <div class="pedal-knobs ${effect.knobs.length === 3 ? 'three-knobs' : ''}">${effect.knobs.map(knob => {
      const value = pedal.controls?.[knob] ?? 50;
      return `<div class="knob-group"><span class="knob-label">${knob}</span>
        <div class="effect-knob" role="slider" tabindex="0" data-effect-id="${pedal.id}" data-knob="${knob}"
          aria-label="${effect.name} ${knob}" aria-valuemin="0" aria-valuemax="100" aria-valuenow="${value}"
          style="--turn:${dialTurn(value, 0, 100)}"></div></div>`;
    }).join('')}</div>
    <div class="effect-status"><span class="led"></span>${pedal.active ? 'ENGAGED' : 'BYPASSED'}</div>
    ${concept !== 'wood' ? `<button class="editor-button" data-vst-add="${pedal.id}">ADD VST3</button>` : ''}
    <button class="bypass-button" data-bypass="${pedal.id}" aria-label="${pedal.active ? 'Bypass' : 'Engage'} ${effect.name}" aria-pressed="${pedal.active}"></button>
    <span class="footswitch-label">${pedal.active ? 'BYPASS' : 'ENGAGE'}</span>
    ${moveTools(pedal.id, effect.name, index, true)}</article>`;
}

function addEffect(type, slot = state.chain.findIndex(pedal => pedal.type === 'looper') + 1) {
  if (!Object.hasOwn(effects, type)) return;
  if (state.chain.length >= (study ? 11 : 7)) return notify(`This preview fits up to ${study ? 'ten' : 'six'} demo effects. No real plug-ins are loaded.`);
  const pedal = { id: `${type}-${nextId++}`, type, active: true,
    controls: Object.fromEntries(effects[type].knobs.map(knob => [knob, 50])) };
  state.chain.splice(Math.max(0, Math.min(slot, state.chain.length)), 0, pedal);
  render();
  notify(`${effects[type].name} added to the pedalboard.`);
}

function updateEffectKnob(pedalId, knob, value, element) {
  const pedal = state.chain.find(item => item.id === pedalId);
  if (!pedal || !effects[pedal.type].knobs.includes(knob)) return;
  const adjusted = Math.max(0, Math.min(100, value));
  pedal.controls ??= {};
  pedal.controls[knob] = adjusted;
  element.setAttribute('aria-valuenow', adjusted);
  element.style.setProperty('--turn', dialTurn(adjusted, 0, 100));
}

function dialText(name, value) {
  const control = footerControls[name.slice(6)];
  return name.startsWith('board-') ? `${value > 0 && control.min < 0 ? '+' : ''}${value} ${control.unit}` : `${value}%`;
}

function dialTurn(value, min, max) {
  return `${(value - min) * 270 / (max - min) - 135}deg`;
}

function levelDial(name, value, position) {
  const boardControl = name.startsWith('board-');
  const label = boardControl ? `Board ${name.slice(6)}` : `Deck ${name} level`;
  const { min = 0, max = 100 } = boardControl ? footerControls[name.slice(6)] : {};
  return `<div class="deck-dial level-dial ${position === 'strip' ? 'strip-dial' : ''}" data-level="${name}" role="slider" tabindex="0"
    aria-label="${label}" aria-valuemin="${min}" aria-valuemax="${max}"
    aria-valuenow="${value}" aria-valuetext="${dialText(name, value)}" style="--turn:${dialTurn(value, min, max)}"
    title="Drag up/down or use arrow keys to adjust"></div>`;
}

function updateLevel(name, value) {
  const footer = name.startsWith('board-');
  const key = footer ? name.slice(6) : name;
  const { min = 0, max = 100 } = footer ? footerControls[key] : {};
  const level = Math.max(min, Math.min(max, Math.round(value)));
  (footer ? state.footer : state.levels)[key] = level;
  document.querySelectorAll(`[data-level="${name}"]`).forEach(control => {
    if (control.matches('input')) control.value = level;
    else {
      control.setAttribute('aria-valuenow', level);
      control.setAttribute('aria-valuetext', dialText(name, level));
      control.style.setProperty('--turn', dialTurn(level, min, max));
    }
    control.nextElementSibling.textContent = control.matches('input') || !footer
      ? `${level}%` : dialText(name, level);
  });
}

function looperMarkup(index) {
  const action = state.recording ? 'FINISH TAKE' : !state.layers.length ? 'RECORD' : !state.playing ? 'PLAY' : state.layered ? 'ADD LAYER' : 'OVERDUB';
  const status = state.recording ? 'RECORDING DEMO' : !state.layers.length ? 'READY TO RECORD' : state.playing ? 'PLAYING' : 'STOPPED';
  const wood = concept === 'wood';
  const controls = wood ? `<div class="deck-controls">${Object.entries(state.levels).map(([name, value]) =>
    `<div class="deck-control"><span>${name.toUpperCase()}</span>${levelDial(name, value, 'deck')}<span class="level-value">${value}%</span></div>`).join('')}</div>` : '';
  return `<article class="pedal looper ${state.recording ? 'recording' : ''} ${state.playing ? '' : 'is-stopped'}" data-pedal="looper">
    <span class="pedal-jacks" aria-hidden="true"></span>
    ${wood ? '<i class="case-corner top-left"></i><i class="case-corner top-right"></i><i class="case-corner bottom-left"></i><i class="case-corner bottom-right"></i>' : ''}
    <div class="drag-handle" ${pedalHandle('looper', 'looper')}><div class="pedal-number"><span>0${index + 1} / LOOP ENGINE</span><span>STEREO</span></div><h3 class="pedal-title">${wood ? 'Loop Deck' : concept === 'stage' ? 'LOOP / LIVE' : 'LOOP STATION'}</h3></div>
    ${controls}${wood ? '<div class="deck-face"><div class="tape-reel" aria-hidden="true"></div>' : ''}
    <div class="loop-screen"><div class="progress-ring" style="--progress:${state.progress}" aria-hidden="true"></div><div class="loop-readout"><strong>${status}</strong>${wood
      ? `<span class="tempo-readout">${state.bpm}<small>BPM</small></span><span class="signature-readout">${state.beats}/${state.unit}</span>`
      : `<span>${state.layers.length ? '08.0' : '00.0'}</span><small>SECONDS</small>`}</div></div>
    ${wood ? '<div class="tape-reel" aria-hidden="true"></div></div>' : ''}
    <div class="footswitches"><div class="foot-group"><button class="foot-button" id="record-demo" aria-label="${action.toLowerCase()} demo"></button><span>${action}</span></div><div class="foot-group"><button class="foot-button" id="stop-demo" aria-label="Stop demo" ${!state.layers.length && !state.recording ? 'disabled' : ''}></button><span>STOP</span></div>${wood ? '' : `<div class="foot-group"><button class="foot-button" id="undo-demo" aria-label="Undo or redo demo take" ${!state.undo || state.recording ? 'disabled' : ''}></button><span>UNDO / REDO</span></div>`}</div>
    <div class="looper-utility"><span>${state.layers.length} ${state.layers.length === 1 ? 'LAYER' : 'LAYERS'} · ${state.metronome ? 'CLICK ON' : 'CLICK OFF'}</span><button id="clear-demo">CLEAR</button></div>
    ${moveTools('looper', 'looper', index)}</article>`;
}

function renderLayers() {
  $('#layer-count').textContent = `${state.layers.length} / ${state.layered ? 8 : 1}`;
  $('#empty-layers').hidden = state.layers.length !== 0;
  $('#layers').innerHTML = state.layers.map((layer, index) => {
    const waveform = Array.from({ length: 26 }, (_, i) => `<i style="--height:${5 + ((i * 13 + layer.id * 7 + (layer.take ?? 0) * 11) % 23)}px"></i>`).join('');
    const colors = concept === 'wood' ? ['#bf925f', '#a89ba4', '#b6a58b', '#9e8b82', '#9394a6', '#c4a581', '#bb8a8a', '#aca39a'] : palettes;
    return `<div class="layer ${layer.active ? '' : 'inactive'} ${layer.solo ? 'soloed' : ''}" style="--layer-color:${colors[index % colors.length]}" data-layer="${layer.id}">
      <div class="layer-id"><span>${String(index + 1).padStart(2, '0')}</span><div><strong>${layer.name}</strong><small>RECORDED · 8.0s</small></div></div>
      <div class="waveform" aria-hidden="true">${waveform}</div>
      <label class="layer-slider"><span>LEVEL <output>${layer.volume}%</output></span><input data-volume="${layer.id}" aria-label="Layer ${index + 1} volume" type="range" min="0" max="100" value="${layer.volume}"></label>
      <label class="layer-slider"><span>PAN <output>${panLabel(layer.pan)}</output></span><input data-pan="${layer.id}" aria-label="Layer ${index + 1} pan" type="range" min="-100" max="100" value="${layer.pan}"></label>
      <div class="layer-actions"><button data-active="${layer.id}" aria-pressed="${layer.active}">${layer.active ? 'ON' : 'OFF'}</button><button data-solo="${layer.id}" aria-pressed="${layer.solo}">SOLO</button><button data-rerecord="${layer.id}" ${state.recording ? 'disabled' : ''}>RETAKE</button><button class="delete-button" data-delete="${layer.id}" aria-label="Delete layer ${index + 1}" ${state.recording ? 'disabled' : ''}>DELETE</button></div></div>`;
  }).join('');
}

function panLabel(pan) { return pan === 0 ? 'C' : `${pan < 0 ? 'L' : 'R'}${Math.abs(pan)}`; }

const studyArtRatios = {};
function studyArtRatio(type) {
  if (studyArtRatios[type] === undefined) {
    const artwork = document.createElement('template');
    artwork.innerHTML = type === 'looper' ? $('.gallery-looper').outerHTML : galleryPedal(effects[type] ? type : 'boost');
    const body = artwork.content.querySelector('[data-gallery-body]');
    studyArtRatios[type] = (Number(body.getAttribute('width')) + 24) / Number(body.getAttribute('height'));
  }
  return studyArtRatios[type];
}

function studyGroups(order, view = study, page = studyPage) {
  const loop = order.findIndex(pedal => pedal.type === 'looper');
  if (view === 'board-1') return [order.slice(0, loop), order.slice(loop + 1)];
  if (view === 'board-3') return [page === 'pre' ? order.slice(0, loop) : order.slice(loop + 1)];
  return [order];
}

function studyRows(pedals, count, height, gap) {
  const widths = [0];
  pedals.forEach(pedal => widths.push(widths[widths.length - 1] + studyArtRatio(pedal.type) * height));
  const memo = new Map();
  const split = (start, remaining) => {
    const key = `${start}-${remaining}`;
    if (memo.has(key)) return memo.get(key);
    if (remaining === 1) return { width: widths[pedals.length] - widths[start] + (pedals.length - start - 1) * gap, rows: [pedals.slice(start)] };
    let best;
    for (let end = start + 1; end <= pedals.length - remaining + 1; end++) {
      const rest = split(end, remaining - 1);
      const width = Math.max(widths[end] - widths[start] + (end - start - 1) * gap, rest.width);
      if (!best || width <= best.width) best = { width, rows: [pedals.slice(start, end), ...rest.rows] };
    }
    memo.set(key, best);
    return best;
  };
  const rows = pedals.length ? split(0, Math.min(count, pedals.length)).rows : [];
  while (rows.length < count) rows.push([]);
  return rows;
}

function layoutStudyBoard(order, view = study, page = studyPage) {
  const board = $('#board');
  const focused = board.contains(document.activeElement) ? document.activeElement : null;
  const viewport = $('.board-viewport');
  const viewportWidth = viewport.clientWidth;
  if (!viewportWidth) return;
  for (const name of ['--study-side-min', '--study-deck-gap', '--study-deck-width']) board.style.removeProperty(name);
  const style = getComputedStyle(board);
  const paddingHeight = parseFloat(style.paddingTop) + parseFloat(style.paddingBottom);
  const value = name => parseFloat(style.getPropertyValue(name));
  const available = viewportWidth - value('padding-left') - value('padding-right') - 1;
  const gap = value('--study-row-gap');
  const rowGap = parseFloat(getComputedStyle(board.querySelector('.study-side')).rowGap) || 0;
  let sideMin = value('--study-side-min');
  let deckGap = value('--study-deck-gap');
  let deckWidth = value('--study-deck-width');
  if (view === 'board-1') {
    const scale = Math.min(1, available / (2 * (sideMin + deckGap) + deckWidth));
    sideMin *= scale;
    deckGap *= scale;
    deckWidth *= scale;
    board.style.setProperty('--study-side-min', `${sideMin}px`);
    board.style.setProperty('--study-deck-gap', `${deckGap}px`);
    board.style.setProperty('--study-deck-width', `${deckWidth}px`);
  }
  const centerHeight = view === 'board-1' ? board.querySelector('.study-center').scrollHeight : 0;
  const pageHeight = Math.max(1, Math.min(510, innerHeight - 48), centerHeight + paddingHeight);
  board.style.height = `${pageHeight}px`;
  board.style.minHeight = `${pageHeight}px`;
  const verticalSpace = pageHeight - paddingHeight;
  const groups = studyGroups(order, view, page);
  const requiredWidth = (count, height) => {
    const widths = groups.map(group => Math.max(sideMin, ...studyRows(group, count, height, gap).map(row =>
      row.reduce((sum, pedal) => sum + studyArtRatio(pedal.type) * height, 0) + Math.max(0, row.length - 1) * gap)));
    return view === 'board-1' ? widths[0] + widths[1] + deckWidth + 2 * deckGap : widths[0];
  };
  const maxRows = Math.max(1, ...groups.map(group => group.length));
  let count = 1, height = 0;
  for (let rows = 1; rows <= maxRows; rows++) {
    const high = (verticalSpace - rowGap * (rows - 1)) / (rows * 1.15);
    if (high <= 0) continue;
    let low = Math.min(studyMinPedalHeight, high);
    if (requiredWidth(rows, low) > available) continue;
    let upper = high;
    for (let i = 0; i < 16; i++) {
      const middle = (low + upper) / 2;
      if (requiredWidth(rows, middle) <= available) low = middle;
      else upper = middle;
    }
    if (low > height) { count = rows; height = low; }
  }
  if (!height) height = Math.max(1, (verticalSpace - rowGap * (count - 1)) / (count * 1.15));
  board.dataset.studyRails = count;
  board.style.setProperty('--study-row-height', `${height * 1.15}px`);
  const extraRails = [...board.querySelectorAll('.study-extra-rail')];
  const extraCount = Math.max(0, count - 2);
  while (extraRails.length < extraCount) {
    const rail = document.createElement('div');
    rail.className = 'rail study-extra-rail';
    rail.setAttribute('aria-hidden', 'true');
    rail.style.top = `calc(var(--study-rail-${extraRails.length + 3}) - 29px)`;
    board.insertBefore(rail, $('#pedals'));
    extraRails.push(rail);
  }
  extraRails.slice(extraCount).forEach(rail => rail.remove());
  const sides = board.querySelectorAll('.study-side');
  groups.forEach((group, groupIndex) => {
    const rows = [...sides[groupIndex].querySelectorAll('.study-row')];
    while (rows.length < count) {
      const row = document.createElement('div');
      row.className = 'study-row';
      sides[groupIndex].append(row);
      rows.push(row);
    }
    rows.forEach((row, index) => { row.hidden = index >= count; });
    studyRows(group, count, height, gap).forEach((pedals, rowIndex) => {
      pedals.forEach(pedal => {
        const card = pedal.id ? board.querySelector(`[data-study-card="${pedal.id}"]`) : board.querySelector('.study-drop-placeholder');
        if (!card) return;
        if (pedal.id) sizeGalleryArt(card.querySelector('svg'), height);
        else sizeGalleryArt(card.querySelector('svg'), height);
        rows[rowIndex].append(card);
      });
      if (focused && focused.isConnected && document.activeElement !== focused) focused.focus({ preventScroll: true });
    });
  });
}

function renderStudyBoard() {
  const loopIndex = state.chain.findIndex(pedal => pedal.type === 'looper');
  const mini = pedal => {
    const name = pedal.type === 'looper' ? 'LOOP DECK' : effects[pedal.type].name;
    const canMove = pedal.type !== 'looper' || study === 'board-2';
    return `<div class="study-mini-wrap" data-study-card="${pedal.id}">
      <div class="study-mini" data-pedal="${pedal.id}">
        ${pedal.type === 'looper' ? $('.gallery-looper').outerHTML.replaceAll('looper-finish', 'study-looper-finish').replaceAll('looper-metal', 'study-looper-metal') : galleryPedal(pedal.type).replaceAll(`finish-${pedal.type}`, `study-finish-${pedal.id}`)}
        ${canMove ? `<button type="button" class="study-mini-handle" data-study-move="${pedal.id}"
          aria-label="Move ${name}. Drag or use Left and Right arrow keys." title="Drag to move ${name}"></button>` : ''}
        <button type="button" class="study-mini-open" data-study-expand="${pedal.id}"
          aria-label="Open ${name} controls" title="Click to open ${name} controls"></button>
      </div>
    </div>`;
  };
  const rows = pedals => `<div class="study-row">${pedals.map(mini).join('')}</div><div class="study-row"></div>`;
  const pre = state.chain.slice(0, loopIndex);
  const post = state.chain.slice(loopIndex + 1);
  if (study === 'board-1') {
    $('#pedals').innerHTML = `<div class="study-side" data-study-side="pre"><span class="study-side-label">PRE / RECORDED</span>${rows(pre)}</div>
      <div class="study-center">${looperMarkup(loopIndex)}</div>
      <div class="study-side" data-study-side="post"><span class="study-side-label">POST / PLAYBACK</span>${rows(post)}</div>`;
  } else if (study === 'board-3') {
    const pedals = studyPage === 'pre' ? pre : post;
    $('#pedals').innerHTML = `<div class="study-side study-paged-chain">${rows(pedals)}</div>`;
  } else {
    $('#pedals').innerHTML = `<div class="study-side study-full-chain"><span class="study-side-label">SIGNAL CHAIN / DRAG OR USE ARROW KEYS TO REORDER</span>${rows(state.chain)}</div>`;
  }
  layoutStudyBoard(state.chain);
  if (expandedPedal) {
    const index = state.chain.findIndex(pedal => pedal.id === expandedPedal);
    if (index < 0) { expandedPedal = null; $('#study-pedal-dialog').close(); }
    else $('#study-expanded-pedal').innerHTML = state.chain[index].type === 'looper' ? looperMarkup(index) : effectMarkup(state.chain[index], index);
  }
}

function drawStudyCables(order = studyDrag?.order || state.chain) {
  const board = $('#board');
  const svg = $('#cables');
  const origin = board.getBoundingClientRect();
  const cards = order.filter(pedal => pedal.id)
    .map(pedal => board.querySelector(`[data-pedal="${pedal.id}"]`)).filter(Boolean);
  const tuner = $('#wood-tuner').getBoundingClientRect();
  const wire = path => `<path class="study-cable-shadow" d="${path}"/><path class="study-cable" d="${path}"/><path class="study-cable-highlight" d="${path}"/>`;
  const straightPlug = (x, y, direction, width = 42) => `<image href="assets/black-plugged.png" x="${-width}" y="${-width * 80 / 384}" width="${width}" height="${width * 80 / 192}" transform="translate(${x} ${y}) scale(${-direction} 1)"/>`;
  const toolbar = $('.board-toolbar').getBoundingClientRect();
  const y = tuner.top + tuner.height / 2 - toolbar.top;
  const tunerLeft = tuner.left - toolbar.left, tunerRight = tuner.right - toolbar.left;
  const inputY = y + tuner.width * .035, outputY = y - tuner.width * .035;
  const boardTop = origin.top - toolbar.top, boardInputX = tunerRight + 90;
  $('#study-tuner-cable').innerHTML = wire(`M -28 ${inputY + 10} C -10 ${inputY + 10}, ${tunerLeft - 60} ${inputY}, ${tunerLeft - 42} ${inputY} M ${tunerRight + 42} ${outputY} C ${boardInputX} ${outputY}, ${boardInputX} ${boardTop - 20}, ${boardInputX} ${boardTop}`)
    + straightPlug(tunerLeft, inputY, -1) + straightPlug(tunerRight, outputY, 1);
  svg.setAttribute('viewBox', `0 0 ${board.clientWidth} ${board.clientHeight}`);
  const boxes = cards.map(card => {
    const box = (card.querySelector('[data-gallery-body]') || card).getBoundingClientRect();
    return { left: box.left - origin.left, right: box.right - origin.left, top: box.top - origin.top, bottom: box.bottom - origin.top,
      x: (box.left + box.right) / 2 - origin.left, y: box.top + box.height * .55 - origin.top };
  });
  const anglePlug = (x, y, direction, vertical, width) => `<image href="assets/right-angle-plugged.png" x="${-width}" y="-21" width="${width}" height="66" preserveAspectRatio="none" transform="translate(${x} ${y}) scale(${-direction} ${vertical})"/>`;
  const plugs = [];
  const links = boxes.slice(0, -1).map((a, index) => {
    const b = boxes[index + 1];
    const sameRow = Math.abs(a.y - b.y) < 10;
    const x1 = a.right, x2 = b.left;
    const group = content => `<g data-cable-from="${cards[index].dataset.pedal}" data-cable-to="${cards[index + 1].dataset.pedal}">${content}</g>`;
    const vertical = sameRow ? -1 : Math.sign(b.y - a.y);
    const endVertical = sameRow ? -1 : -vertical;
    const plugWidth = sameRow ? Math.min(20, Math.abs(x2 - x1) / 2) : 20;
    const startX = x1 + plugWidth / 2, startY = a.y + vertical * 45;
    const endX = x2 - plugWidth / 2, endY = b.y + endVertical * 45;
    const bend = sameRow ? Math.min(50, Math.max(0, startY - 12)) : Math.max(24, Math.abs(endY - startY) * .35);
    let path = `M ${startX} ${startY} C ${startX} ${startY + vertical * bend}, ${endX} ${endY + endVertical * bend}, ${endX} ${endY}`;
    if (b.top > a.bottom) {
      const middleY = (a.bottom + b.top) / 2;
      path = `M ${startX} ${startY} C ${startX} ${middleY - 12}, ${startX} ${middleY}, ${startX - 12} ${middleY} H ${endX + 12} C ${endX} ${middleY}, ${endX} ${middleY + 12}, ${endX} ${endY}`;
    }
    plugs.push(anglePlug(x1, a.y, 1, vertical, plugWidth), anglePlug(x2, b.y, -1, endVertical, plugWidth));
    return group(wire(path));
  }).join('');
  let ends = '';
  if (boxes.length) {
    const first = boxes[0], last = boxes[boxes.length - 1];
    const inputEnd = first.left - 42;
    const inputX = toolbar.left + boardInputX - origin.left;
    const input = `M ${inputX} 0 C ${inputX} ${first.y / 2}, ${inputEnd - 35} ${first.y}, ${inputEnd} ${first.y}`;
    const outputStart = last.right + 42;
    const outputSpan = board.clientWidth - outputStart;
    const output = `M ${outputStart} ${last.y} C ${outputStart + outputSpan / 3} ${last.y}, ${outputStart + outputSpan * 2 / 3} ${last.y + 24}, ${board.clientWidth} ${last.y + 24}`;
    ends = `<g data-cable-to="${cards[0].dataset.pedal}">${wire(input)}</g>`
      + `<g data-cable-from="${cards[cards.length - 1].dataset.pedal}">${wire(output)}</g>`;
    plugs.push(straightPlug(first.left, first.y, -1), straightPlug(last.right, last.y, 1));
  }
  svg.innerHTML = ends + links + plugs.join('');
}

function render() {
  reorderDrag = null;
  document.querySelectorAll('[data-study-page]').forEach(button => button.setAttribute('aria-pressed', button.dataset.studyPage === studyPage));
  $('#wood-layers').textContent = `Layers (${state.layers.length})`;
  const layeredButton = $('#layered-setting');
  layeredButton.disabled = !!state.layers.length || state.recording;
  layeredButton.setAttribute('aria-pressed', state.layered);
  layeredButton.title = `Layered loop ${state.layered ? 'on' : 'off'}`;
  const metronomeButton = $('#click-setting');
  metronomeButton.setAttribute('aria-pressed', state.metronome);
  metronomeButton.title = `Metronome ${state.metronome ? 'on' : 'off'}`;
  $('#tempo-tap').disabled = state.recording;
  for (const [id, value] of [['bpm-setting', state.bpm], ['beats-setting', state.beats],
    ['unit-setting', state.unit], ['count-setting', state.count], ['loop-bars-setting', state.autoStopBars]]) {
    const control = $(`#${id}`);
    if (document.activeElement !== control) control.value = id === 'bpm-setting' ? Number(value).toFixed(2) : value;
  }
  $('#count-setting').disabled = !!state.layers.length || state.recording;
  $('#loop-bars-setting').disabled = !!state.layers.length || state.recording;
  renderLibrary();
  if (study && (study !== 'board-3' || studyPage !== 'loop')) renderStudyBoard();
  else {
    const loopIndex = state.chain.findIndex(pedal => pedal.type === 'looper');
    $('#pedals').innerHTML = state.chain.map((pedal, index) => {
      if (study === 'board-3' && (studyPage === 'pre' ? index >= loopIndex : studyPage === 'post' ? index <= loopIndex : index !== loopIndex)) return '';
      return pedal.type === 'looper' ? looperMarkup(index) : effectMarkup(pedal, index);
    }).join('');
  }
  $('#chain-order').innerHTML = state.chain.map(pedal => `<span class="${pedal.type === 'looper' ? 'loop-order' : ''}">${pedal.type === 'looper' ? 'LOOPER' : effects[pedal.type].name}</span>`).join('');
  if (concept === 'wood') {
    const controls = Object.entries(footerControls).map(([name, control]) => `<div class="board-control">
      <b>${name.toUpperCase()}</b>${levelDial(`board-${name}`, state.footer[name], 'strip')}<span class="level-value">${dialText(`board-${name}`, state.footer[name])}</span>
      ${control.switchable ? `<button class="board-switch" role="switch" aria-label="${name}" aria-checked="${state[name]}" data-board-switch="${name}">${state[name] ? 'ON' : 'OFF'}</button>` : ''}
    </div>`);
    const loopControls = study === 'board-2' ? `<div class="study-footer-loop" aria-label="Looper quick controls">
      <strong>LOOP DECK</strong>${Object.entries(state.levels).map(([name, value]) => `<div class="board-control"><b>${name.toUpperCase()}</b>${levelDial(name, value, 'strip')}<span class="level-value">${value}%</span></div>`).join('')}
      <div class="study-transport"><button type="button" data-study-transport="record-demo">${state.recording ? 'Finish' : 'Record / play'}</button><button type="button" data-study-transport="stop-demo" ${!state.layers.length && !state.recording ? 'disabled' : ''}>Stop</button><button type="button" data-study-transport="clear-demo">Clear</button></div>
      <div class="study-timing"><label> BPM <input type="number" data-study-setting="bpm-setting" min="30" max="300" step="1" value="${state.bpm}"></label>
      <label> BEATS <input type="number" data-study-setting="beats-setting" min="1" max="16" value="${state.beats}"></label>
      <label> UNIT <select data-study-setting="unit-setting">${[1, 2, 4, 8, 16].map(unit => `<option ${unit === state.unit ? 'selected' : ''}>${unit}</option>`).join('')}</select></label></div></div>` : '';
    $('#wood-footer').innerHTML = `<div class="board-controls">${controls.slice(0, 3).join('')}</div>${loopControls}<div class="board-controls">${controls.slice(3).join('')}</div>`;
    $('#wood-tuner').setAttribute('aria-pressed', state.tuner);
  } else {
    document.querySelectorAll('.level-strip input[data-level]').forEach(input => updateLevel(input.dataset.level, state.levels[input.dataset.level]));
  }
  renderLayers();
  positionPedals();
  if (study) drawCables();
  else requestAnimationFrame(drawCables);
}

function positionPedals() {
  const board = $('#board');
  if (study) {
    board.style.minWidth = '100%';
    if (study === 'board-3' && studyPage === 'loop') delete board.dataset.studyRails;
    if (study === 'board-3' && studyPage === 'loop') {
      board.style.height = '';
      board.style.minHeight = '';
    }
    return;
  }
  board.style.height = '';
  board.style.minHeight = '';
  if (concept !== 'canvas') {
    const stage = concept === 'stage';
    const wood = concept === 'wood';
    if (wood) {
      board.style.minWidth = `max(100%, calc(${state.chain.length} * var(--pedal-width) + ${(state.chain.length - 1) * 22 + 80}px))`;
      return;
    }
    const width = state.chain.reduce((total, pedal) => total + (pedal.type === 'looper'
      ? stage ? 420 : 269 : stage ? 155 : 145), 0);
    board.style.minWidth = `${Math.max(stage ? 970 : 770, width + (state.chain.length - 1) * (stage ? 50 : 22) + 80)}px`;
    return;
  }
  state.chain.forEach((pedal, index) => {
    const occupied = state.chain.filter(item => item.id !== pedal.id).map(item => state.positions[item.id]?.x).filter(x => x !== undefined);
    const position = state.positions[pedal.id] ??= { x: occupied.length ? Math.max(...occupied) + 250 : 35,
      y: pedal.type === 'looper' ? 45 : index % 2 ? 170 : 120 };
    const element = document.querySelector(`[data-pedal="${pedal.id}"]`);
    element.style.left = `${position.x}px`;
    element.style.top = `${position.y}px`;
  });
  board.style.minWidth = `${Math.max(770, ...state.chain.map(pedal => state.positions[pedal.id].x
    + (pedal.type === 'looper' ? 235 : 145) + 35))}px`;
}

function renderPatchCables(board, svg, pedals) {
  if (!board.clientWidth || pedals.length < 2) {
    svg.innerHTML = '';
    return;
  }
  const origin = board.getBoundingClientRect();
  const boxes = pedals.map(pedal => pedal.getBoundingClientRect());
  const width = Math.max(board.clientWidth, board.scrollWidth);
  svg.setAttribute('viewBox', `0 0 ${width} ${board.clientHeight}`);
  const links = boxes.slice(0, -1).map((box, index) => {
    const next = boxes[index + 1];
    const startX = box.right - origin.left - 2;
    const endX = next.left - origin.left + 2;
    const startY = box.top - origin.top + box.height * .55;
    const endY = next.top - origin.top + next.height * .55;
    const cableStart = startX + 10;
    const cableEnd = endX - 10;
    const span = Math.max(1, cableEnd - cableStart);
    const sag = 32 + Math.min(24, span * .35);
    const path = `M ${cableStart} ${startY} C ${cableStart + span * .3} ${startY + sag}, ${cableEnd - span * .3} ${endY + sag}, ${cableEnd} ${endY}`;
    return `<g class="patch-link">
      <path class="patch-cable-shadow" d="${path}"/>
      <path class="patch-cable" d="${path}"/>
      <path class="patch-cable-highlight" d="${path}"/>
      <g class="patch-plug" transform="translate(${startX} ${startY})">
        <rect x="-2" y="-5" width="11" height="10" rx="2"/>
        <rect x="7" y="-2" width="5" height="4" rx="1"/>
      </g>
      <g class="patch-plug" transform="translate(${endX} ${endY}) scale(-1 1)">
        <rect x="-2" y="-5" width="11" height="10" rx="2"/>
        <rect x="7" y="-2" width="5" height="4" rx="1"/>
      </g>
    </g>`;
  }).join('');
  svg.innerHTML = `<defs><linearGradient id="patch-metal"><stop stop-color="#77736b"/><stop offset=".4" stop-color="#e5e2d7"/><stop offset=".7" stop-color="#8c8981"/><stop offset="1" stop-color="#d8d5ca"/></linearGradient></defs>${links}`;
}

function drawCables() {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (study) {
    const board = $('#board');
    const origin = board.getBoundingClientRect();
    const side = board.querySelector('.study-mini')?.closest('.study-side') || board.querySelector('.study-side');
    const looper = board.querySelector('.pedal.looper');
    let centers;
    if (side) {
      centers = [...side.querySelectorAll('.study-row:not([hidden])')].map(row => {
        const bounds = row.getBoundingClientRect();
        return bounds.top - origin.top + bounds.height / 2;
      });
    } else if (looper) {
      const bounds = looper.getBoundingClientRect();
      centers = [bounds.top - origin.top + bounds.height / 4, bounds.top - origin.top + bounds.height * 3 / 4];
    }
    if (centers) centers.forEach((center, index) => board.style.setProperty(`--study-rail-${index + 1}`, `${center}px`));
  }
  if (!demoCablesEnabled) {
    $('#cables').replaceChildren();
    $('#study-tuner-cable').replaceChildren();
    return;
  }
  if (study) { drawStudyCables(); return; }
  const pedals = state.chain.map(pedal => document.querySelector(`[data-pedal="${pedal.id}"]`)).filter(Boolean);
  renderPatchCables($('#board'), $('#cables'), pedals);
}

function tunerNotes(note) {
  return ['A', 'A#', 'B', 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#']
    .map(name => {
      const label = name.endsWith('#') ? '·' : name;
      return name === note ? `<strong>${label}</strong>` : label;
    }).join(' ');
}

function askConfirmation(title, description, action) {
  $('#confirm-title').textContent = title;
  $('#confirm-description').textContent = description;
  $('#confirm-accept').textContent = title.startsWith('Clear') ? 'Clear demo'
    : title.startsWith('Load') ? 'Load' : title.startsWith('Replace') ? 'Replace' : 'Delete';
  confirmation = action;
  $('#confirm-dialog').showModal();
}

document.addEventListener('click', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const button = event.target.closest('button');
  if (concept === 'wood' && state.toolbox && button?.id !== 'wood-toolbox' && !event.target.closest('.library')) setToolbox(false);
  if (!button || button.disabled) return;
  const data = button.dataset;
  if (data.settingsTab) {
    document.querySelectorAll('[data-settings-tab]').forEach(tab => {
      const selected = tab === button;
      tab.setAttribute('aria-selected', selected);
      tab.tabIndex = selected ? 0 : -1;
    });
    $('#audio-panel').hidden = data.settingsTab !== 'audio';
    $('#visual-panel').hidden = data.settingsTab !== 'visual';
  } else if (data.studyPage) {
    studyPage = data.studyPage;
    render();
  } else if (data.studyExpand) {
    expandedPedal = data.studyExpand;
    render();
    $('#study-pedal-dialog').showModal();
  } else if (data.studyClose !== undefined) {
    $('#study-pedal-dialog').close();
  } else if (data.category) {
    state.effectCategory = state.effectCategory === data.category ? '' : data.category;
    renderLibrary();
    document.querySelectorAll('.library-folder').forEach(folder => {
      if (folder.dataset.category === data.category) folder.focus();
    });
  } else if (data.addType) {
    addEffect(data.addType, study === 'board-3' && studyPage === 'pre' ? state.chain.findIndex(pedal => pedal.type === 'looper') : undefined);
  } else if (data.bypass) {
    const pedal = state.chain.find(item => item.id === data.bypass); pedal.active = !pedal.active; render();
  } else if (data.vstAdd !== undefined) {
    notify('This browser preview cannot load VST3s. In the desktop app, add a plugin from this pedal.');
  }
  else if (data.move) {
    const index = state.chain.findIndex(pedal => pedal.id === data.move);
    const next = index + Number(data.direction);
    [state.chain[index], state.chain[next]] = [state.chain[next], state.chain[index]];
    render(); notify('Demo signal order changed. In the real app, pre-loop effects become part of the recorded take.');
  } else if (data.removePedal) {
    const index = [...$('#pedals').querySelectorAll('[data-study-expand]')]
      .findIndex(edit => edit.dataset.studyExpand === data.removePedal);
    state.chain = state.chain.filter(pedal => pedal.id !== data.removePedal);
    render();
    if (study) {
      const edits = [...$('#pedals').querySelectorAll('[data-study-expand]')];
      (edits[Math.min(index, edits.length - 1)] ?? (study === 'board-3'
        ? $('[data-study-page][aria-pressed="true"]') : $('#wood-toolbox')))?.focus();
      notify('Demo pedal deleted.');
    }
  }
  else if (data.active || data.solo) {
    const layer = state.layers.find(item => item.id === Number(data.active || data.solo));
    if (data.active) layer.active = !layer.active; else layer.solo = !layer.solo;
    renderLayers();
  } else if (data.rerecord) {
    state.undo = { before: structuredClone(state.layers), after: null, applied: false };
    const layer = state.layers.find(item => item.id === Number(data.rerecord));
    layer.take = (layer.take ?? 0) + 1;
    state.undo.after = structuredClone(state.layers);
    render();
    notify(concept === 'wood' ? 'Demo waveform replaced. The real app keeps the old take until capture finishes.'
      : 'Demo waveform replaced. The real app keeps the old take until capture finishes; undo restores it.');
  } else if (data.delete) {
    const id = Number(data.delete);
    askConfirmation('Delete demo layer?', 'Other demo takes and their mixer settings will be kept.'
      + (concept === 'wood' ? '' : ' This clears demo undo/redo history.'), () => {
      state.layers = state.layers.filter(layer => layer.id !== id); state.undo = null;
      if (!state.layers.length) { state.playing = false; state.progress = 0; }
      render(); notify('Demo layer deleted. The remaining layers have been renumbered.');
    });
  } else if ((data.studyTransport || button.id) === 'record-demo') {
    if (state.recording) {
      state.undo = { before: structuredClone(state.layers), after: null, applied: false };
      if (!state.layered && state.layers.length) {
        state.layers[0].take = (state.layers[0].take ?? 0) + 1;
        notify('Demo overdub added into the same layer. No audio was recorded.');
      }
      else state.layers.push({ id: nextId++, name: `New take ${state.layers.length + 1}`, volume: 80, pan: 0, active: true, solo: false });
      state.undo.after = structuredClone(state.layers); state.recording = false; state.playing = true;
    } else if (state.layers.length && !state.playing) state.playing = true;
    else {
      if (state.layered && state.layers.length === 8) return notify('Eight layers are full. Delete a take or rerecord an existing one.');
      state.recording = true; state.playing = true;
      notify(!state.layers.length && state.count ? `Preview: ${state.count}-bar count-in selected. Press Finish Take to create a demo layer.` : 'Recording animation only. Press Finish Take to commit a demo layer.');
    }
    render();
  } else if ((data.studyTransport || button.id) === 'stop-demo') { state.playing = false; state.recording = false; state.progress = 0; render(); }
  else if (button.id === 'undo-demo' && state.undo) {
    state.undo.applied = !state.undo.applied;
    state.layers = structuredClone(state.undo.applied ? state.undo.before : state.undo.after);
    if (!state.layers.length) state.playing = false;
    render();
  } else if ((data.studyTransport || button.id) === 'clear-demo') askConfirmation('Clear all demo takes?', 'This clears the preview only. It cannot affect recordings in the real Looper app.', () => {
    state.layers = []; state.recording = false; state.playing = false; state.undo = null; state.progress = 0; render();
  });
  else if (button.id === 'layout-reset') {
    state.positions = {}; $('#board').style.minWidth = ''; positionPedals(); requestAnimationFrame(drawCables); notify('Pedal positions reset. Processing order and takes are unchanged.');
  } else if (button.id === 'wood-toolbox') {
    setToolbox(!state.toolbox);
    if (state.toolbox) $('#effect-search').focus();
  } else if (button.id === 'close-effects') setToolbox(false, true);
  else if (button.id === 'wood-layers') $('#layers-dialog').showModal();
  else if (button.id === 'wood-tuner') {
    state.tuner = !state.tuner;
    button.setAttribute('aria-pressed', state.tuner);
  } else if (data.boardSwitch) {
    const enabled = state[data.boardSwitch] = !state[data.boardSwitch];
    button.setAttribute('aria-checked', enabled);
    button.textContent = enabled ? 'ON' : 'OFF';
  } else if (button.id === 'layered-setting') {
    if (!button.disabled) { state.layered = !state.layered; render(); }
  } else if (button.id === 'click-setting') {
    state.metronome = !state.metronome; render();
  } else if (button.id === 'tempo-tap') {
    const now = performance.now();
    if (lastTempoTap && now - lastTempoTap < 2000) {
      state.bpm = Math.max(30, Math.min(300, Math.round(60000 / (now - lastTempoTap))));
      render();
    }
    lastTempoTap = now;
  } else if (button.id === 'demo-cables-toggle') {
    demoCablesEnabled = !demoCablesEnabled;
    button.setAttribute('aria-pressed', demoCablesEnabled);
    button.textContent = demoCablesEnabled ? 'Hide cables' : 'Show cables';
    drawCables();
  } else if (button.id === 'settings-open') {
    $('#settings-dialog').showModal();
  } else if (button.id === 'save-board-preset') {
    $('#preset-name').value = state.presetName || '';
    $('#preset-dialog').returnValue = '';
    $('#preset-dialog').showModal();
    $('#preset-name').focus();
  } else if (data.closeDialog !== undefined) $('#settings-dialog').close();
  else if (button.id === 'confirm-cancel') { confirmation = null; $('#confirm-dialog').close(); }
  else if (button.id === 'confirm-accept') { const action = confirmation; confirmation = null; $('#confirm-dialog').close(); if (action) action(); }
});

function updateDemoPresets() {
  const select = $('#board-preset');
  select.replaceChildren(new Option('Unsaved board', ''));
  if (state.presetName) select.add(new Option('Reload current preset', 'reload'));
  for (const name of Object.keys(demoPresets[concept] || {}).sort())
    select.add(new Option(name, `preset:${name}`));
  select.value = state.presetName ? `preset:${state.presetName}` : '';
}

$('#preset-dialog').addEventListener('close', event => {
  if (document.documentElement.classList.contains('juce-hosted') || event.target.returnValue !== 'save') return;
  const name = $('#preset-name').value.trim();
  if (!name) return notify('Enter a preset name.');
  const save = () => {
    const { chain, levels, footer, gate, doubler, tuner, bpm, beats, unit, count, autoStopBars, metronome, layered } = state;
    (demoPresets[concept] ??= Object.create(null))[name] = structuredClone({ chain, levels, footer, gate, doubler, tuner, bpm, beats, unit, count, autoStopBars, metronome, layered });
    state.presetName = name;
    updateDemoPresets();
    notify('Demo preset saved for this preview session.');
  };
  if (Object.hasOwn(demoPresets[concept] || {}, name))
    askConfirmation('Replace demo preset?', 'The saved demo preset will be replaced.', save);
  else save();
});
$('#preset-dialog').addEventListener('keydown', event => {
  if (event.key !== 'Escape') return;
  event.preventDefault();
  event.stopPropagation();
  $('#preset-dialog').close();
});
$('#board-preset').addEventListener('change', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const name = event.target.value === 'reload' ? state.presetName : event.target.value.slice(7);
  updateDemoPresets();
  if (!name) return;
  askConfirmation('Load demo preset?', 'The demo board will be replaced. Demo takes are kept.', () => {
    Object.assign(state, structuredClone(demoPresets[concept][name]));
    state.presetName = name;
    updateDemoPresets();
    render();
    notify('Demo preset loaded.');
  });
});

document.addEventListener('input', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const input = event.target;
  if (input.dataset.volume || input.dataset.pan) {
    const layer = state.layers.find(item => item.id === Number(input.dataset.volume || input.dataset.pan));
    if (input.dataset.volume) layer.volume = Number(input.value); else layer.pan = Number(input.value);
    input.closest('label').querySelector('output').textContent = input.dataset.volume ? `${layer.volume}%` : panLabel(layer.pan);
  } else if (input.dataset.level) {
    updateLevel(input.dataset.level, Number(input.value));
  }
});
document.addEventListener('keydown', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (event.key === 'Escape' && studyPointer) {
    event.preventDefault();
    finishStudyPointer({ pointerId: studyPointer.pointer, type: 'pointercancel' });
    return;
  }
  if (event.key === 'Escape' && concept === 'wood' && state.toolbox) {
    setToolbox(false, true);
    return;
  }
  const mini = event.target.closest('[data-study-move], [data-study-expand]');
  if (study && mini?.closest('[data-study-card]')?.querySelector('[data-study-move]') && ['ArrowLeft', 'ArrowRight'].includes(event.key)) {
    event.preventDefault();
    const id = mini.dataset.studyMove || mini.dataset.studyExpand;
    const focusAttribute = mini.dataset.studyMove ? 'data-study-move' : 'data-study-expand';
    const from = state.chain.findIndex(pedal => pedal.id === id);
    const to = from + (event.key === 'ArrowLeft' ? -1 : 1);
    if (study === 'board-3' && state.chain[to]?.type === 'looper') studyPage = studyPage === 'pre' ? 'post' : 'pre';
    moveStudyPedal(from, to);
    $(`[${focusAttribute}="${id}"]`)?.focus();
    return;
  }
  const handle = event.target.closest('[data-drag]');
  if (concept === 'wood' && handle && ['ArrowLeft', 'ArrowRight'].includes(event.key)) {
    event.preventDefault();
    const id = handle.dataset.drag;
    reorderPedal(id, state.chain.findIndex(pedal => pedal.id === id) + (event.key === 'ArrowLeft' ? -1 : 1));
    return;
  }
  const effectKnob = event.target.closest('.effect-knob');
  if (effectKnob) {
    const value = Number(effectKnob.getAttribute('aria-valuenow'));
    const step = event.shiftKey ? 5 : 1;
    const changes = { ArrowUp: value + step, ArrowRight: value + step, ArrowDown: value - step,
      ArrowLeft: value - step, PageUp: value + 10, PageDown: value - 10,
      Home: 0, End: 100 };
    if (!Object.hasOwn(changes, event.key)) return;
    event.preventDefault();
    updateEffectKnob(effectKnob.dataset.effectId, effectKnob.dataset.knob, changes[event.key], effectKnob);
    return;
  }
  const dial = event.target.closest('.level-dial');
  if (!dial) return;
  const value = Number(dial.getAttribute('aria-valuenow'));
  const step = event.shiftKey ? 5 : 1;
  const changes = { ArrowUp: value + step, ArrowRight: value + step, ArrowDown: value - step,
    ArrowLeft: value - step, PageUp: value + 10, PageDown: value - 10,
    Home: Number(dial.getAttribute('aria-valuemin')), End: Number(dial.getAttribute('aria-valuemax')) };
  if (!Object.hasOwn(changes, event.key)) return;
  event.preventDefault();
  updateLevel(dial.dataset.level, changes[event.key]);
});
$('#layers-dialog').addEventListener('keydown', event => {
  if (event.key !== 'Escape') return;
  event.preventDefault();
  $('#layers-dialog').close();
});
document.addEventListener('pointerdown', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const effectKnob = event.target.closest('.effect-knob');
  if (effectKnob && !dialDrag && event.button === 0) {
    dialDrag = { control: effectKnob, pointer: event.pointerId, effectId: effectKnob.dataset.effectId,
      knob: effectKnob.dataset.knob, y: event.clientY, value: Number(effectKnob.getAttribute('aria-valuenow')) };
    effectKnob.focus();
    effectKnob.setPointerCapture(event.pointerId);
    event.preventDefault();
    return;
  }
  const dial = event.target.closest('.level-dial');
  if (!dial || dialDrag || event.button !== 0) return;
  dialDrag = { control: dial, pointer: event.pointerId, name: dial.dataset.level,
    y: event.clientY, value: Number(dial.getAttribute('aria-valuenow')) };
  dial.focus();
  dial.setPointerCapture(event.pointerId);
  event.preventDefault();
});
document.addEventListener('pointermove', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (!dialDrag || event.pointerId !== dialDrag.pointer) return;
  if (dialDrag.effectId) updateEffectKnob(dialDrag.effectId, dialDrag.knob,
    dialDrag.value + (dialDrag.y - event.clientY) / 2, dialDrag.control);
  else updateLevel(dialDrag.name, dialDrag.value + (dialDrag.y - event.clientY) / 2);
});
function finishDialDrag(event) {
  if (!dialDrag || event.pointerId !== dialDrag.pointer) return;
  if (dialDrag.control.hasPointerCapture(event.pointerId)) dialDrag.control.releasePointerCapture(event.pointerId);
  dialDrag = null;
}
document.addEventListener('pointerup', finishDialDrag);
document.addEventListener('pointercancel', finishDialDrag);
document.addEventListener('lostpointercapture', finishDialDrag);
$('#effect-search').addEventListener('input', () => {
  if (!document.documentElement.classList.contains('juce-hosted')) renderLibrary();
});
$('#effect-library').addEventListener('dragstart', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const pedal = event.target.closest('[data-effect-drag]');
  if (!pedal) return;
  event.dataTransfer.setData('application/x-looper-effect', pedal.dataset.effectDrag);
  event.dataTransfer.effectAllowed = 'copy';
  $('#board').classList.add('accepting-drop');
  if (study && !(study === 'board-3' && studyPage === 'loop')) startStudyDrag(null, pedal.dataset.effectDrag);
});
$('#effect-library').addEventListener('dragend', () => {
  $('#board').classList.remove('accepting-drop', 'drag-over');
  finishStudyDrag();
});
$('#board').addEventListener('dragover', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (studyDrag) return;
  if (!Array.from(event.dataTransfer.types).includes('application/x-looper-effect')) return;
  event.preventDefault();
  event.dataTransfer.dropEffect = 'copy';
  $('#board').classList.add('drag-over');
});
$('#board').addEventListener('dragleave', event => {
  if (!$('#board').contains(event.relatedTarget)) $('#board').classList.remove('drag-over');
});
$('#board').addEventListener('drop', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (studyDrag) return;
  const type = event.dataTransfer.getData('application/x-looper-effect');
  if (!Object.hasOwn(effects, type)) return;
  event.preventDefault();
  $('#board').classList.remove('accepting-drop', 'drag-over');
  if (study === 'board-3' && state.chain.length < 11) studyPage = 'post';
  addEffect(type, state.chain.length);
});
document.addEventListener('change', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const input = event.target;
  if (input.id === 'visual-view' && ['board-1', 'board-2', 'board-3'].includes(input.value)) {
    location.hash = input.value;
    return;
  }
  if (input.dataset.studySetting) {
    if (!input.checkValidity()) { input.reportValidity(); return; }
    const original = $(`#${input.dataset.studySetting}`);
    original.value = input.value;
    original.dispatchEvent(new Event('change', { bubbles: true }));
    return;
  }
  if (!input.id.endsWith('-setting')) return;
  if (!input.checkValidity()) { input.reportValidity(); return; }
  if (input.id === 'bpm-setting') state.bpm = Number(input.value);
  if (input.id === 'beats-setting') state.beats = Number(input.value);
  if (input.id === 'unit-setting') state.unit = Number(input.value);
  if (input.id === 'count-setting') state.count = Number(input.value);
  if (input.id === 'loop-bars-setting') state.autoStopBars = Number(input.value);
  render();
});

function moveStudyPedal(from, to) {
  if (from < 0 || to < 0 || to >= state.chain.length || from === to) return;
  const [pedal] = state.chain.splice(from, 1);
  state.chain.splice(to, 0, pedal);
  render();
  notify('Demo signal order changed.');
}

function startStudyDrag(id, type) {
  if (id) type = state.chain.find(pedal => pedal.id === id).type;
  studyDrag = { id, type, to: id ? state.chain.findIndex(pedal => pedal.id === id) : -1 };
  const placeholder = document.createElement('div');
  placeholder.className = 'study-drop-placeholder';
  placeholder.setAttribute('aria-hidden', 'true');
  const artwork = document.createElement('template');
  artwork.innerHTML = type === 'looper' ? $('.gallery-looper').outerHTML : galleryPedal(type);
  const svg = artwork.content.querySelector('svg').cloneNode(false);
  svg.append(artwork.content.querySelector('[data-gallery-body]').cloneNode(true));
  placeholder.append(svg);
  if (id) {
    const source = $(`[data-study-card="${id}"]`);
    source.classList.add('study-drag-source');
  }
  $('#pedals .study-row').append(placeholder);
  if (id) previewStudyDrop(studyDrag.to);
  else sizeGalleryArt(svg, parseFloat(getComputedStyle($('#board')).getPropertyValue('--study-row-height')) / 1.15);
}

function previewStudyDrop(to) {
  const source = studyDrag.id && state.chain.findIndex(pedal => pedal.id === studyDrag.id);
  if (to < 0 || to > state.chain.length - (studyDrag.id ? 1 : 0)) return;
  if (studyDrag.to === to && studyDrag.order) return;
  const cards = [...$('#pedals').querySelectorAll('[data-study-card]:not(.study-drag-source) .study-mini')];
  const positions = cards.map(card => card.getBoundingClientRect());
  cards.forEach(card => card.getAnimations().forEach(animation => animation.cancel()));
  studyDrag.to = to;
  const order = state.chain.slice();
  if (studyDrag.id) order.splice(source, 1);
  order.splice(to, 0, { id: null, type: studyDrag.type });
  studyDrag.order = order;
  layoutStudyBoard(order);
  drawCables();
  if (!reducedMotion) cards.forEach((card, index) => {
    const next = card.getBoundingClientRect();
    const x = positions[index].left - next.left, y = positions[index].top - next.top;
    if (Math.abs(x) > .5 || Math.abs(y) > .5)
      card.animate([{ transform: `translate(${x}px, ${y}px)` }, { transform: 'translate(0, 0)' }],
        { duration: 200, easing: 'ease-out' });
  });
}

function finishStudyDrag() {
  if (!studyDrag) return;
  studyDrag = null;
  $('#board').classList.remove('accepting-drop', 'drag-over');
  $('[data-study-edge]').hidden = true;
  render();
}

$('#study-pedal-dialog').addEventListener('close', () => {
  const id = expandedPedal;
  expandedPedal = null;
  $('#study-expanded-pedal').innerHTML = '';
  if (id && study) $(`[data-study-expand="${id}"]`)?.focus();
});
function updateStudyDrop(event, pointer = studyPointer) {
  const viewport = $('.board-viewport').getBoundingClientRect();
  const board = $('#board').getBoundingClientRect();
  const edge = $('[data-study-edge]');
  const inside = event.clientX >= Math.max(0, viewport.left, board.left)
    && event.clientX <= Math.min(innerWidth, viewport.right, board.right)
    && event.clientY >= Math.max(0, viewport.top, board.top)
    && event.clientY <= Math.min(innerHeight, viewport.bottom, board.bottom);
  if (!inside) { edge.hidden = true; return false; }
  const transfer = study === 'board-3' && studyDrag.id && (studyPage === 'pre'
    ? event.clientX >= Math.min(viewport.right, innerWidth) - 65
    : event.clientX <= Math.max(viewport.left, 0) + 65);
  edge.hidden = !transfer;
  if (transfer) {
    edge.style.left = `${(studyPage === 'pre' ? Math.min(viewport.right, innerWidth) - 65 : Math.max(viewport.left, 0)) - board.left}px`;
    edge.textContent = studyPage === 'pre' ? 'DROP TO POST' : 'DROP TO PRE';
    return true;
  }
  if (!studyDrag.id) {
    previewStudyDrop(state.chain.length);
    return true;
  }
  const cards = [...$('#pedals').querySelectorAll('[data-study-card]')].filter(card => card.dataset.studyCard !== studyDrag.id);
  const side = [...$('#pedals').querySelectorAll('[data-study-side]')].find(item => {
    const rect = item.getBoundingClientRect();
    return event.clientX >= rect.left && event.clientX <= rect.right && event.clientY >= rect.top && event.clientY <= rect.bottom;
  });
  if (study === 'board-1' && side && !cards.some(card => side.contains(card))) {
    const loop = state.chain.findIndex(pedal => pedal.type === 'looper');
    previewStudyDrop(loop + (!studyDrag.id && side.dataset.studySide === 'post' ? 1 : 0));
    return true;
  }
  const x = pointer?.image ? event.clientX - pointer.offsetX + pointer.bodyX : event.clientX;
  const y = pointer?.image ? event.clientY - pointer.offsetY + pointer.bodyY : event.clientY;
  // Keep the destination stable while the target slides away from this center.
  if (studyDrag.target && Math.hypot(x - studyDrag.target.x, y - studyDrag.target.y) <= studyDrag.target.radius) return true;
  studyDrag.target = null;
  const target = cards.find(card => {
    const rect = card.getBoundingClientRect();
    const body = card.querySelector('[data-gallery-body]').getBoundingClientRect();
    const center = { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2,
      radius: Math.min(18, body.width * .12, body.height * .12) };
    if (Math.hypot(x - center.x, y - center.y) > center.radius) return false;
    studyDrag.target = center;
    return true;
  });
  if (target) {
    let to = (studyDrag.id ? studyDrag.order : state.chain).findIndex(pedal => pedal.id === target.dataset.studyCard);
    if (!studyDrag.id && event.clientX > target.getBoundingClientRect().left + target.offsetWidth / 2) to++;
    previewStudyDrop(to);
  } else if (!cards.length) {
    const loop = state.chain.findIndex(pedal => pedal.type === 'looper');
    previewStudyDrop(study === 'board-3' && studyPage === 'pre' ? loop : state.chain.length);
  }
  return true;
}

function commitStudyDrop() {
  const { id, type, to } = studyDrag;
  const transfer = !$('[data-study-edge]').hidden;
  const from = state.chain.findIndex(pedal => pedal.id === id);
  studyDrag = null;
  $('[data-study-edge]').hidden = true;
  $('#board').classList.remove('accepting-drop', 'drag-over');
  if (transfer) {
    studyPage = studyPage === 'pre' ? 'post' : 'pre';
    moveStudyPedal(from, state.chain.findIndex(pedal => pedal.type === 'looper'));
  } else if (id && to >= 0 && from !== to) moveStudyPedal(from, to);
  else if (!id && to >= 0) {
    if (state.chain.length >= 11) render();
    else if (study === 'board-3') studyPage = 'post';
    addEffect(type, to);
  }
  else render();
}

$('#board').addEventListener('dragover', event => {
  if (!studyDrag || studyPointer) return;
  event.preventDefault();
  event.dataTransfer.dropEffect = 'copy';
  updateStudyDrop(event);
});
$('#board').addEventListener('drop', event => {
  if (!studyDrag || studyPointer) return;
  event.preventDefault();
  commitStudyDrop();
});

$('#pedals').addEventListener('pointerdown', event => {
  const handle = event.target.closest('[data-study-move]');
  if (!study || !handle || event.button !== 0 || studyPointer || studyDrag) return;
  studyPointer = { id: handle.dataset.studyMove, pointer: event.pointerId,
    x: event.clientX, y: event.clientY, handle };
  handle.focus({ preventScroll: true });
  // Preview layout hides and moves pedals, so capture on the stable board.
  $('#board').setPointerCapture(event.pointerId);
  event.preventDefault();
});

document.addEventListener('pointermove', event => {
  if (studyPointer?.pointer !== event.pointerId) return;
  const pointer = studyPointer;
  if (!pointer.image) {
    if (Math.hypot(event.clientX - pointer.x, event.clientY - pointer.y) <= 5) return;
    const artwork = pointer.handle.closest('[data-study-card]').querySelector('svg');
    const bounds = artwork.getBoundingClientRect();
    const body = artwork.querySelector('[data-gallery-body]').getBoundingClientRect();
    pointer.offsetX = pointer.x - bounds.left;
    pointer.offsetY = pointer.y - bounds.top;
    pointer.bodyX = body.left + body.width / 2 - bounds.left;
    pointer.bodyY = body.top + body.height / 2 - bounds.top;
    pointer.image = document.createElement('div');
    pointer.image.className = 'study-drag-image';
    pointer.image.setAttribute('aria-hidden', 'true');
    pointer.image.append(artwork.cloneNode(true));
    document.body.append(pointer.image);
    startStudyDrag(pointer.id);
  }
  pointer.image.style.left = `${event.clientX - pointer.offsetX}px`;
  pointer.image.style.top = `${event.clientY - pointer.offsetY}px`;
  updateStudyDrop(event);
  event.preventDefault();
});

function finishStudyPointer(event) {
  if (studyPointer?.pointer !== event.pointerId) return;
  const pointer = studyPointer;
  studyPointer = null;
  pointer.image?.remove();
  if ($('#board').hasPointerCapture(pointer.pointer)) $('#board').releasePointerCapture(pointer.pointer);
  if (!pointer.image) return;
  if (event.type === 'pointerup' && updateStudyDrop(event, pointer)) commitStudyDrop();
  else finishStudyDrag();
}
document.addEventListener('pointerup', finishStudyPointer);
document.addEventListener('pointercancel', finishStudyPointer);
$('#board').addEventListener('lostpointercapture', finishStudyPointer);

function reorderPedal(id, destination, preventScroll = false) {
  const index = state.chain.findIndex(pedal => pedal.id === id);
  if (index < 0 || destination === index || destination < 0 || destination >= state.chain.length) return;
  const [pedal] = state.chain.splice(index, 1);
  state.chain.splice(destination, 0, pedal);
  render();
  $(`[data-drag="${id}"]`).focus({ preventScroll });
  notify('Demo signal order changed.');
}

function updateReorder(scroll = false) {
  const { card, x, y, startX, startY, startScroll } = reorderDrag;
  const viewport = $('.board-viewport');
  const bounds = viewport.getBoundingClientRect();
  const inside = x >= Math.max(0, bounds.left) && x <= Math.min(document.documentElement.clientWidth, bounds.right)
    && y >= bounds.top && y <= bounds.bottom;
  if (inside && scroll) {
    if (x < bounds.left + 45) viewport.scrollLeft -= 12;
    else if (x > bounds.right - 45) viewport.scrollLeft += 12;
  }
  card.style.translate = `${x - startX + viewport.scrollLeft - startScroll}px ${y - startY}px`;
  const others = [...$('#pedals').children].filter(pedal => pedal !== card);
  let slot = 0;
  for (const pedal of others) {
    const rect = pedal.getBoundingClientRect();
    if (x > rect.left + rect.width / 2) slot++;
    else break;
  }
  for (const pedal of others) pedal.classList.remove('drop-before', 'drop-after');
  reorderDrag.slot = inside ? slot : -1;
  if (inside && state.chain[slot].id !== card.dataset.pedal) {
    (others[slot] ?? others.at(-1)).classList.add(slot < others.length ? 'drop-before' : 'drop-after');
  }
}

$('#pedals').addEventListener('pointerdown', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  const handle = event.target.closest('[data-drag]');
  if (!handle || event.button !== 0) return;
  const id = handle.dataset.drag;
  const element = handle.closest('[data-pedal]');
  if (concept === 'wood') {
    if (reorderDrag || state.chain.length < 2) return;
    reorderDrag = { card: element, pointer: event.pointerId, startX: event.clientX, startY: event.clientY,
      startScroll: $('.board-viewport').scrollLeft, x: event.clientX, y: event.clientY, slot: -1 };
    handle.focus({ preventScroll: true });
    handle.setPointerCapture(event.pointerId);
    event.preventDefault();
    return;
  }
  if (concept !== 'canvas') return;
  drag = { id, x: event.clientX, y: event.clientY, left: element.offsetLeft, top: element.offsetTop, pointer: event.pointerId };
  handle.setPointerCapture(event.pointerId);
  event.preventDefault();
});
$('#pedals').addEventListener('pointermove', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  if (reorderDrag?.pointer === event.pointerId) {
    reorderDrag.x = event.clientX;
    reorderDrag.y = event.clientY;
    const card = reorderDrag.card;
    if (!card.classList.contains('reordering')) {
      if (Math.hypot(event.clientX - reorderDrag.startX, event.clientY - reorderDrag.startY) <= 5) return;
      card.getAnimations().forEach(animation => animation.cancel());
      card.classList.add('reordering');
    }
    updateReorder();
    return;
  }
  if (!drag || event.pointerId !== drag.pointer) return;
  const element = document.querySelector(`[data-pedal="${drag.id}"]`);
  state.positions[drag.id] = {
    x: Math.max(25, Math.min($('#board').clientWidth - element.offsetWidth - 25, drag.left + event.clientX - drag.x)),
    y: Math.max(20, Math.min($('#board').clientHeight - element.offsetHeight - 20, drag.top + event.clientY - drag.y))
  };
  positionPedals(); drawCables();
});
function finishReorder(event) {
  if (reorderDrag?.pointer !== event.pointerId) return;
  const { card, slot } = reorderDrag;
  const id = card.dataset.pedal;
  const lifted = card.classList.contains('reordering');
  const from = lifted && !reducedMotion ? card.getBoundingClientRect() : null;
  reorderDrag = null;
  card.style.translate = '';
  card.classList.remove('reordering');
  for (const pedal of $('#pedals').children) pedal.classList.remove('drop-before', 'drop-after');
  if (event.type === 'pointerup') reorderPedal(id, slot, true);
  if (!from) return;
  const landing = $(`[data-pedal="${id}"]`);
  const to = landing.getBoundingClientRect();
  landing.animate([
    { transformOrigin: 'top left', transform: `translate(${from.left - to.left}px, ${from.top - to.top}px) scale(${from.width / to.width})`, zIndex: 2 },
    { transformOrigin: 'top left', transform: 'none', zIndex: 2 }
  ], { duration: 210, easing: 'cubic-bezier(.2,.8,.25,1)' });
}
$('#pedals').addEventListener('pointerup', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  finishReorder(event);
  if (drag) notify('Position changed; signal order stayed the same.');
  drag = null;
});
$('#pedals').addEventListener('pointercancel', event => {
  if (document.documentElement.classList.contains('juce-hosted')) return;
  finishReorder(event); drag = null;
});
$('#pedals').addEventListener('lostpointercapture', event => {
  if (!document.documentElement.classList.contains('juce-hosted')) finishReorder(event);
});
window.addEventListener('hashchange', setConcept);
window.addEventListener('resize', () => {
  if (study && !(study === 'board-3' && studyPage === 'loop') && !studyDrag) layoutStudyBoard(state.chain);
  drawCables();
});
$('.board-viewport').addEventListener('scroll', drawCables);
new ResizeObserver(drawCables).observe($('#board'));
let studyViewportWidth = 0;
new ResizeObserver(() => {
  const width = $('.board-viewport').clientWidth;
  if (width === studyViewportWidth) return;
  studyViewportWidth = width;
  if (study && !studyDrag && !(study === 'board-3' && studyPage === 'loop')) {
    layoutStudyBoard(state.chain);
    drawCables();
  }
}).observe($('.board-viewport'));

function animate(time) {
  if (reorderDrag?.card.classList.contains('reordering')) updateReorder(true);
  const elapsed = Math.min(time - frameTime, 100) / 1000;
  frameTime = time;
  if (!document.documentElement.classList.contains('juce-hosted') && state && state.playing && !reducedMotion) {
    state.progress = (state.progress + elapsed / 8) % 1;
    const ring = $('.progress-ring'); if (ring) ring.style.setProperty('--progress', state.progress);
  }
  requestAnimationFrame(animate);
}
setConcept();
requestAnimationFrame(animate);
