#include <WiFi.h>
#include <WebServer.h>

// --- DONANIM PIN TANIMLAMALARI ---
const int MOTOR_1_IA = 13; // Sol İleri
const int MOTOR_1_IB = 14; // Sol Geri
const int MOTOR_2_IA = 25; // Sağ İleri
const int MOTOR_2_IB = 26; // Sağ Geri

const int BUZZER_PIN = 32; 

// --- ELEKTRİKSEL AYARLAR ---
const int freq_motor = 500;   
const int freq_buzzer = 2000; 
const int resolution = 8;    

const int ch_1a = 0; const int ch_1b = 1;
const int ch_2a = 2; const int ch_2b = 3;
const int ch_buzzer = 4;

const int drivePower = 180; 

// --- ANKARA DOLMUŞ KORNASI ("Darari Darari Darari") ---
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659

int dolmusMelody[] = {
  NOTE_C5, NOTE_D5, NOTE_E5, // Da-ra-ri
  NOTE_C5, NOTE_D5, NOTE_E5, // Da-ra-ri
  NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_C5, NOTE_C5  // Da-ra-ri
};

int noteDurations[] = {
  80, 80, 100,
  80, 80, 100,
  80, 80, 100,
  120, 120// Son ses biraz daha uzun yankılanır
};

const int totalNotes = 11;

// Asenkron Melodi Takip Değişkenleri
int currentNote = -1;
unsigned long noteStartTime = 0;
bool isResting = false;

WebServer server(80);
const char* ssid = "Android Auto";
const char* password = "123";

// --- YATAY MODA, SABİT SLIDER VE ACİL STOP UYUMLU COCKPIT UI ---
const char html_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-select=no">
    <title>Remote Car Control</title>
    <style>
        * { box-sizing: border-box; -webkit-user-select: none; user-select: none; }
        body {
            background-color: #030811; color: #d8e3fb; font-family: 'Courier New', monospace;
            margin: 0; padding: 0; overflow: hidden; height: 100vh; width: 100vw;
            display: flex; flex-direction: column; touch-action: none;
        }
        header {
            height: 35px; background: rgba(4, 14, 31, 0.9); border-bottom: 1px solid #1f2a3c;
            display: flex; align-items: center; justify-content: center; z-index: 10;
        }
        .logo { color: #00F0FF; font-weight: 900; letter-spacing: 2px; text-shadow: 0 0 8px #00F0FF; font-size: 14px;}
        
        .landscape-warning {
            display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            background: #030811; color: #fff; text-align: center; justify-content: center; align-items: center;
            z-index: 1000; font-size: 18px; flex-direction: column;
        }
        
        .dashboard {
            display: grid; grid-template-columns: 1fr 1fr 1fr;
            flex: 1; padding: 10px; align-items: center; height: calc(100vh - 35px);
        }
        .panel { 
            background: rgba(21, 32, 49, 0.15); border: 1px solid #1f2a3c; 
            border-radius: 12px; height: 95%; display: flex; align-items: center; justify-content: center; position: relative;
        }
        
        input[type="range"] {
            -webkit-appearance: none; appearance: none;
            background: #121e30; border-radius: 6px;
            outline: none; border: 1px solid #1f2a3c;
        }
        
        #steer-slider { width: 80%; height: 12px; }
        #steer-slider::-webkit-slider-thumb {
            -webkit-appearance: none; appearance: none;
            width: 30px; height: 30px; border-radius: 6px; background: #00f0ff;
            box-shadow: 0 0 10px #00f0ff80; cursor: pointer;
        }
        
        .slider-wrapper {
            height: 150px; width: 60px; display: flex; align-items: center; justify-content: center; position: relative;
        }
        #throttle-slider {
            width: 150px; height: 12px;
            transform: rotate(-90deg); 
        }
        #throttle-slider::-webkit-slider-thumb {
            -webkit-appearance: none; appearance: none;
            width: 30px; height: 30px; border-radius: 6px; background: #ff3b3b;
            box-shadow: 0 0 10px #ff3b3b80; cursor: pointer;
        }

        .center-panel { display: flex; flex-direction: column; align-items: center; gap: 6px;}
        .stat-box { border: 1px solid #00f0ff20; padding: 4px; width: 85%; border-radius: 6px; text-align: center; background: rgba(4,14,31,0.5); }
        .label { font-size: 9px; color: #849495; letter-spacing: 1px;}
        .value { color: #fff; font-size: 14px; font-weight: bold;}
        
        .btn {
            background: rgba(0, 240, 255, 0.03); border: 1px solid #00f0ff; color: #00f0ff;
            padding: 8px; font-size: 11px; font-weight: bold; border-radius: 6px; width: 85%; cursor: pointer;
        }
        .btn-horn { border-color: #ff9800; color: #ff9800; }
        .btn-horn:active { background: rgba(255,152,0,0.15); }
        
        .btn-stop { border-color: #ff3b3b; color: #ff3b3b; margin-top: 4px; }
        .btn-stop:active { background: rgba(255,59,59,0.15); }

        @media (orientation: portrait) {
            .landscape-warning { display: flex; }
            .dashboard { display: none; }
        }
    </style>
</head>
<body>

<div class="landscape-warning">
    <div style="font-size: 40px;">&#128242;</div>
    <div style="margin-top:10px;">Sürüş için telefonu yan çevirin.</div>
</div>

<header>
    <div class="logo">Remote Car Control</div>
</header>

<div class="dashboard">
    <div class="panel">
        <div style="width: 100%; display: flex; justify-content: center;">
            <input id="steer-slider" type="range" min="-100" max="100" value="0">
        </div>
    </div>

    <div class="center-panel">
        <div class="stat-box">
            <div class="label">STEER DATA</div>
            <div class="value" id="steer-val">0</div>
        </div>
        <div class="stat-box">
            <div class="label">THROTTLE DATA</div>
            <div class="value" id="throttle-val">0</div>
        </div>
        <button class="btn btn-horn" id="horn-btn" onclick="playHorn()">HORN</button>
        <button class="btn btn-stop" id="stop-btn" onclick="emergencyStop()">EMERGENCY STOP</button>
    </div>

    <div class="panel">
        <div class="slider-wrapper">
            <input id="throttle-slider" type="range" min="-180" max="180" value="0">
        </div>
    </div>
</div>

<script>
    let throttle = 0;
    let steering = 0;
    let isHornPlaying = false;

    function sendData() {
        fetch(`/control?t=${throttle}&s=${steering}`).catch(e => console.log(e));
    }

    const sSlider = document.getElementById('steer-slider');
    sSlider.addEventListener('input', (e) => {
        steering = parseInt(e.target.value);
        document.getElementById('steer-val').textContent = steering;
        sendData();
    });

    const tSlider = document.getElementById('throttle-slider');
    tSlider.addEventListener('input', (e) => {
        throttle = parseInt(e.target.value);
        document.getElementById('throttle-val').textContent = throttle;
        sendData();
    });

    function playHorn() {
        if(isHornPlaying) return;
        isHornPlaying = true;
        const btn = document.getElementById('horn-btn');
        btn.textContent = "PLAYING...";
        
        fetch('/horn_play').catch(e => console.log(e));
        
        // Yeni melodiye göre buton sıfırlama süresi (Ortalama 2.2 saniye)
        setTimeout(() => {
            if(isHornPlaying) {
                btn.textContent = "HORN";
                isHornPlaying = false;
            }
        }, 2200);
    }

    // ACİL DURDURMA FONKSİYONU
    function emergencyStop() {
        throttle = 0;
        steering = 0;
        document.getElementById('throttle-slider').value = 0;
        document.getElementById('steer-slider').value = 0;
        document.getElementById('throttle-val').textContent = "0";
        document.getElementById('steer-val').textContent = "0";
        
        sendData(); // Motorları anında keser
        
        fetch('/horn_stop').catch(e => console.log(e)); // Kornayı anında keser
        isHornPlaying = false;
        document.getElementById('horn-btn').textContent = "HORN";
    }
</script>
</body>
</html>
)=====";

// --- ESP32 ARKA PLAN MANTIĞI ---

void handleControl() {
  int t = 0; 
  int s = 0; 

  if (server.hasArg("t")) t = server.arg("t").toInt();
  if (server.hasArg("s")) s = server.arg("s").toInt();

  int steeringValue = (s * drivePower) / 100;

  int leftMotorSpeed = t + steeringValue;
  int rightMotorSpeed = t - steeringValue;

  leftMotorSpeed = constrain(leftMotorSpeed, -drivePower, drivePower);
  rightMotorSpeed = constrain(rightMotorSpeed, -drivePower, drivePower);

  if (leftMotorSpeed > 0) {
    ledcWrite(ch_1a, leftMotorSpeed); ledcWrite(ch_1b, 0);
  } else if (leftMotorSpeed < 0) {
    ledcWrite(ch_1a, 0); ledcWrite(ch_1b, abs(leftMotorSpeed));
  } else {
    ledcWrite(ch_1a, 0); ledcWrite(ch_1b, 0);
  }

  if (rightMotorSpeed > 0) {
    ledcWrite(ch_2a, rightMotorSpeed); ledcWrite(ch_2b, 0);
  } else if (rightMotorSpeed < 0) {
    ledcWrite(ch_2a, 0); ledcWrite(ch_2b, abs(rightMotorSpeed));
  } else {
    ledcWrite(ch_2a, 0); ledcWrite(ch_2b, 0);
  }

  server.send(200, "text/plain", "OK");
}

void handleHornPlay() {
  currentNote = 0;
  isResting = false;
  noteStartTime = millis();
  ledcWriteTone(ch_buzzer, dolmusMelody[currentNote]);
  server.send(200, "text/plain", "OK"); 
}

void handleHornStop() {
  currentNote = -1; // Asenkron melodiyi iptal eder
  ledcWriteTone(ch_buzzer, 0); 
  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  server.send_P(200, "text/html", html_page);
}

void setup() {
  Serial.begin(115200);

  ledcSetup(ch_1a, freq_motor, resolution);
  ledcSetup(ch_1b, freq_motor, resolution);
  ledcSetup(ch_2a, freq_motor, resolution);
  ledcSetup(ch_2b, freq_motor, resolution);

  ledcAttachPin(MOTOR_1_IA, ch_1a);
  ledcAttachPin(MOTOR_1_IB, ch_1b);
  ledcAttachPin(MOTOR_2_IA, ch_2a);
  ledcAttachPin(MOTOR_2_IB, ch_2b);

  ledcSetup(ch_buzzer, freq_buzzer, resolution);
  ledcAttachPin(BUZZER_PIN, ch_buzzer);
  ledcWriteTone(ch_buzzer, 0); 

  WiFi.mode(WIFI_AP); 
  WiFi.softAP(ssid, password);
  
  Serial.print("Yatay Kumanda Seti Hazir. IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/horn_play", handleHornPlay);
  server.on("/horn_stop", handleHornStop);
  
  server.begin();
}

void loop() {
  server.handleClient();

  // --- NON-BLOCKING (ASENKRON) KORNA OYNATICISI ---
  if (currentNote >= 0 && currentNote < totalNotes) {
    unsigned long now = millis();
    if (!isResting) {
      if (now - noteStartTime >= noteDurations[currentNote]) {
        ledcWriteTone(ch_buzzer, 0); 
        isResting = true;
        noteStartTime = now;
      }
    } else {
      if (now - noteStartTime >= 50) { 
        currentNote++;
        if (currentNote < totalNotes) {
          ledcWriteTone(ch_buzzer, dolmusMelody[currentNote]);
          isResting = false;
          noteStartTime = now;
        }
      }
    }
  }
}