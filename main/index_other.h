const uint8_t index_simple_html[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title id="title">ESP32-CAM Simplified View</title>
    <link rel="stylesheet" type="text/css" href="/style.css">
    <style>
      @media (min-width: 800px) and (orientation:landscape) {
        #content {
          display:flex;
          flex-wrap: nowrap;
          flex-direction: column;
          align-items: flex-start;
        }
      }
    </style>
  </head>

  <body>
    <section class="main">
      <div id="logo">
        <button id="toggle-stream" style="float:left;">Start Stream</button>
      </div>
      <div id="content">
        <figure>
          <div id="stream-container" class="image-container hidden">
            <div class="close close-rot-none" id="close-stream">×</div>
            <img id="stream" src="">
          </div>
        </figure>
      </div>
    </section>
  </body>

  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
    var baseHost = document.location.origin;
    var streamURL = 'stream';

    
    const view = document.getElementById('stream')
    const viewContainer = document.getElementById('stream-container')
   
    const streamButton = document.getElementById('toggle-stream')
   
    const hide = el => {
      el.classList.add('hidden')
    }
    const show = el => {
      el.classList.remove('hidden')
    }

    const disable = el => {
      el.classList.add('disabled')
      el.disabled = true
    }

    const enable = el => {
      el.classList.remove('disabled')
      el.disabled = false
    }


    const stopStream = () => {
      window.stop();
      streamButton.innerHTML = 'Start Stream';
          streamButton.setAttribute("title", `Start the stream :: ${streamURL}`);
      hide(viewContainer);
    }

    const startStream = () => {
      view.src = streamURL;
      view.scrollIntoView(false);
      streamButton.innerHTML = 'Stop Stream';
      streamButton.setAttribute("title", `Stop the stream`);
      show(viewContainer);
    }
    
    streamButton.onclick = () => {
      const streamEnabled = streamButton.innerHTML === 'Stop Stream'
      if (streamEnabled) {
        stopStream();
      } else {
        startStream();
      }
    }

  })
  </script>
</html>)=====";

size_t index_simple_html_len = sizeof(index_simple_html)-1;

/* Stream Viewer */

const uint8_t streamviewer_html[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title id="title">ESP32-CAM StreamViewer</title>
    <style>
      /* No stylesheet, define all style elements here */
      body {
        font-family: Arial,Helvetica,sans-serif;
        background: #181818;
        color: #EFEFEF;
        font-size: 16px;
        margin: 0px;
        overflow:hidden;
      }

      img {
        object-fit: contain;
        display: block;
        margin: 0px;
        padding: 0px;
        width: 100vw;
        height: 100vh;
      }

      .loader {
        border: 0.5em solid #f3f3f3;
        border-top: 0.5em solid #000000;
        border-radius: 50%;
        width: 1em;
        height: 1em;
        -webkit-animation: spin 2s linear infinite; /* Safari */
        animation: spin 2s linear infinite;
      }

      @-webkit-keyframes spin {   /* Safari */
        0% { -webkit-transform: rotate(0deg); }
        100% { -webkit-transform: rotate(360deg); }
      }

      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }
    </style>
  </head>

  <body>
    <section class="main">
      <div id="wait-settings" style="float:left;" class="loader" title="Waiting for stream settings to load"></div>
      <div style="display: none;">
        <!-- Hide the next entries, they are present in the body so that we
             can pass settings to/from them for use in the scripting -->
        <div id="rotate" class="action-setting hidden">0</div>
        <div id="cam_name" class="action-setting hidden"></div>
        <div id="stream_url" class="action-setting hidden"></div>
      </div>
      <img id="stream" src="">
    </section>
  </body>

  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
    var baseHost = document.location.origin;
    var streamURL = 'Undefined';

    const rotate = document.getElementById('rotate')
    const stream = document.getElementById('stream')
    const spinner = document.getElementById('wait-settings')

    const updateValue = (el, value, updateRemote) => {
      updateRemote = updateRemote == null ? true : updateRemote
      let initialValue
      if (el.type === 'checkbox') {
        initialValue = el.checked
        value = !!value
        el.checked = value
      } else {
        initialValue = el.value
        el.value = value
      }

      if (updateRemote && initialValue !== value) {
        updateConfig(el);
      } else if(!updateRemote){
        if(el.id === "cam_name"){
          window.document.title = value;
          stream.setAttribute("title", value + "\n(doubleclick for fullscreen)");
          console.log('Name set to: ' + value);
        } else if(el.id === "rotate"){
          rotate.value = value;
          console.log('Rotate recieved: ' + rotate.value);
        } else if(el.id === "stream_url"){
          streamURL = value;
          console.log('Stream URL set to:' + value);
        }
      }
    }

    // read initial values
    fetch(`${baseHost}/info`)
      .then(function (response) {
        return response.json()
      })
      .then(function (state) {
        document
          .querySelectorAll('.action-setting')
          .forEach(el => {
            updateValue(el, state[el.id], false)
          })
        spinner.style.display = `none`;
        applyRotation();
        startStream();
      })

    const startStream = () => {
      stream.src = streamURL;
      stream.style.display = `block`;
    }

    const applyRotation = () => {
      rot = rotate.value;
      if (rot == -90) {
        stream.style.transform = `rotate(-90deg)`;
      } else if (rot == 90) {
        stream.style.transform = `rotate(90deg)`;
      }
      console.log('Rotation ' + rot + ' applied');
    }

    stream.ondblclick = () => {
      if (stream.requestFullscreen) {
        stream.requestFullscreen();
      } else if (stream.mozRequestFullScreen) { /* Firefox */
        stream.mozRequestFullScreen();
      } else if (stream.webkitRequestFullscreen) { /* Chrome, Safari and Opera */
        stream.webkitRequestFullscreen();
      } else if (stream.msRequestFullscreen) { /* IE/Edge */
        stream.msRequestFullscreen();
      }
    }
  })
  </script>
</html>)=====";

size_t streamviewer_html_len = sizeof(streamviewer_html)-1;
