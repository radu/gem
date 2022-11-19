// We import the CSS which is extracted to its own file by esbuild.
// Remove this line if you add a your own CSS build pipeline (e.g postcss).
// 
// If you want to use Phoenix channels, run `mix help phx.gen.channel`
// to get started and then uncomment the line below.
// import "./user_socket.js"

// You can include dependencies in two ways.
//
// The simplest option is to put them in assets/vendor and
// import them using relative paths:
//
//     import "./vendor/some-package.js"
//
// Alternatively, you can `npm install some-package` and import
// them using a path starting with the package name:
//
//     import "some-package"
//

// Include phoenix_html to handle method=PUT/DELETE in forms and buttons.
import "phoenix_html"
// Establish Phoenix Socket and LiveView configuration.
import {Socket} from "phoenix"
import {LiveSocket} from "phoenix_live_view"
import Tone from "../vendor/tone.min.js"

import ColorPicker from "simple-color-picker"

let synth = undefined;
let reverb = undefined;
let delay = undefined;

const modeBtnClass = "mode-switch";
const audioBtnId = "audio-btn";
const adsr = "adsr";
const reverbId = "slider_reverb";
const delayId = "slider_delay";
const tempoId = "slider_tempo";
const colorPickerId = "color_picker";

// light/dark mode
const setupLightDarkMode = () => {
  const modeBtns = document.getElementsByClassName(modeBtnClass);
  if (modeBtns.length > 0) {
    for (let i = 0; i < modeBtns.length; i++) {
      modeBtns[i].addEventListener("click", (e) => {
        const body = document.body;
        body.classList.toggle("dark");
      });
    }
  }
}

/**
 * Synth
 *
 * Setup the instrument and effects.
 */ 
if (document.getElementById(audioBtnId)) {
  document.getElementById(audioBtnId).addEventListener("click", function() {
    Tone.context.resume().then(() => {
      console.log('Playback resumed successfully');
      synth = new Tone.PolySynth({
        maxPolyphony: 64,
        voice: Tone.Synth,
        volume: -6,
        options: {
          envelope : {
            attack : 2,
            decay : 1.8,
            sustain : 1,
            release : 10
          }
        }
      })

      reverb = new Tone.Reverb(0.7);
      delay = new Tone.PingPongDelay("3n", 0.5);
      const compressor = new Tone.Compressor(-30, 3);
      
      /**
       * Audio effects chain:
       *
       * [PolySynth] --> [Reverb] --> [Delay] --> [Compressor] --> Output
       */
      synth.connect(reverb);
      reverb.connect(delay);
      delay.connect(compressor);
      compressor.toDestination();

      synth.triggerAttackRelease("C4", "8n");
    });
  });
}

/**
 * Beep!
 *
 * Make noise for each active item in a matrix column
 */
/*
const beep = (col) => {
  let notes = [];
  for (let i = 0; i < col.children.length; i++) {
    if (col.children[i].classList.contains("active")) {
      notes.push(col.children[i].getAttribute("data-note"));
    }
  }
  if (notes.length > 0 && synth) {
    const adsrInputs = document.getElementById(colorpicker).getElementsByTagName("slider");
    const envelope = {};
    for (let i = 0; i < adsrInputs.length; i++) {
      envelope[adsrInputs[i].id] = parseFloat(adsrInputs[i].getAttribute("data-value"))
    }
    synth.set({
      envelope : {
        attack : envelope["r"],
        decay : envelope["g"],
        sustain : envelope["b"],
        release : envelope["a"]
      }
    });

    reverb.set({wet: parseFloat(document.getElementById(reverbId).value)/100.0 || 0.01});
    delay.set({wet: parseFloat(document.getElementById(delayId).value)/100.0});

    synth.triggerAttackRelease(notes, 0.5);
  }
}
*/

/**
 * Tic-toc
 *
 * Increment the sequencer at a certain tempo
 */
/*
const indicators = document.getElementsByClassName("indicator");
const tic = (i) => {
  const col = indicators[(i+1)%indicators.length].parentElement;
  const tempo = parseInt(document.getElementById(tempoId).getAttribute("data-value-ms"));
  indicators[i%indicators.length].classList.remove("active");
  indicators[(i+1)%indicators.length].classList.add("active");
  beep(col);
  setTimeout(tic, tempo, (i+1)%indicators.length);
}
tic(0);
*/

const colorPicker = new ColorPicker();

let Hooks = {}
Hooks.Mounted = {
  mounted() {
    setupLightDarkMode();
  }
}

Hooks.ColorPickerHook = {
  mounted() {
    console.log('ColorPicker mounted');

    if (document.getElementById(colorPickerId)) {
      e = document.getElementById(colorPickerId);
      colorPicker.appendTo(e);

      const view = this;
      
      colorPicker.onChange(function() {
        console.log("Color picked: ", colorPicker.getRGB());
        view.pushEvent("color-select", {"color": colorPicker.getHexString()});
      })
    }
  }
}

let csrfToken = document.querySelector("meta[name='csrf-token']").getAttribute("content")
let liveSocket = new LiveSocket("/live", Socket, {params: {_csrf_token: csrfToken}, hooks: Hooks})

// connect if there are any LiveViews` on the page
liveSocket.connect()

// expose liveSocket on window for web console debug logs and latency simulation:
// >> liveSocket.enableDebug()
// >> liveSocket.enableLatencySim(1000)  // enabled for duration of browser session
// >> liveSocket.disableLatencySim()
window.liveSocket = liveSocket
