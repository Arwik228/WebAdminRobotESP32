var textTemplate = {
    start: "Подключаюсь...",
    open: "Соединение установлено",
    stop: "Соединение закрыто",
    error: "Ошибка соединения",
    reload: "Соединение перезагружается",
    find: "Подаю звуковой сигнал...",
    ready: "Ожидаю команду",
    noselect: "Режим не выбран"
}
var ws, mapArray = [],
    text = textTemplate.start,
    joy, timeout;

function socket() {
    ws = new WebSocket(`ws://${window.location.host || "192.168.1.199"}/websocket`);
    ws.onopen = (e) => {
        document.getElementById("status").innerText = "Online";
        document.getElementById("status").style.color = "lime";
        text = textTemplate.open;
    }
    ws.onclose = (e) => {
        document.getElementById("status").innerText = "Offline";
        document.getElementById("status").style.color = "red";
        text = textTemplate.stop;
    }
    ws.onmessage = (e) => {
        try {
            let message = JSON.parse(e.data);
            let io1 = document.getElementById("io1");
            let io2 = document.getElementById("io2");
            let io3 = document.getElementById("io3");
            let io4 = document.getElementById("io4");
            var clear = () => {
                io1.innerText = io2.innerText = io3.innerText = io4.innerText = "false";
                io1.style.color = io2.style.color = io3.style.color = io4.style.color = "red";
            }
            if (message.center !== undefined) {
                timeout = Date.now();
                message.left == 0 ? (
                    io1.innerText = "false",
                    io1.style.color = "lime"
                ) : (
                    io1.innerText = "true",
                    io1.style.color = "red"
                );
                message.center == 0 ? (
                    io2.innerText = "false",
                    io2.style.color = "lime"
                ) : (
                    io2.innerText = "true",
                    io2.style.color = "red"
                );
                message.right == 0 ? (
                    io3.innerText = "false",
                    io3.style.color = "lime"
                ) : (
                    io3.innerText = "true",
                    io3.style.color = "red"
                );
                message.bumper == 0 ? (
                    io4.innerText = "false",
                    io4.style.color = "lime"
                ) : (
                    io4.innerText = "true",
                    io4.style.color = "red"
                );
            }
        } catch (str) {
            console.log(e.data)
        }
        text = textTemplate.ready;
    }
    ws.onerror = (e) => {
        document.getElementById("status").innerText = "Offline";
        document.getElementById("status").style.color = "red";
        text = textTemplate.error;
    }
}

function reconect() {
    if (ws.close) {
        ws.close(), ws = false, setTimeout(socket, 2500);
    }
}

function find() {
    text = textTemplate.find, ws.send("find");
}

function go() {
    let input = document.querySelector('input[name="contact"]:checked');
    if (input) {
        text = textTemplate.ready
        if (ws.readyState === 1) {
            ws.send("goAuto");
        } else {
            text = textTemplate.error;
        }
    } else {
        text = textTemplate.noselect;
    }
}

function stop() {
    if (ws.readyState === 1) {
        ws.send("stopAuto");
    } else {
        text = textTemplate.error;
    }
}

function goHome() {
    if (ws.readyState === 1) {
        ws.send("goHome");
    } else {
        text = textTemplate.error;
    }
}

function alg(e) {
    let input = document.querySelector('input[name="contact"]:checked');
    if (input) {
        text = textTemplate.ready
        if (ws.readyState === 1) {
            ws.send([`alg-${input.value}`]);
        } else {
            text = textTemplate.error;
        }
    } else {
        text = textTemplate.noselect;
    }
}

function clearStory() {
    if (ws.readyState === 1) {
        ws.send("clearStory");
    } else {
        text = textTemplate.error;
    }
}

function joystickRender() {
    let send = oldValue = oldSpeed = false;
    joy = new JoyStick('joyDiv', {
        title: 'joystick',
        width: 150,
        height: 150,
        internalFillColor: 'orange',
        internalLineWidth: 2,
        internalStrokeColor: 'orange',
        externalLineWidth: 2,
        externalStrokeColor: '#e91e63',
        autoReturnToCenter: true
    });
    setInterval(() => {
        if (ws.readyState === 1) {
            let localJoyX = joy.GetPosX();
            let localJoyY = joy.GetPosY()
            if (75 != localJoyX || 75 != localJoyY) {
                let absX = Math.abs(localJoyX - 75);
                let absY = Math.abs(localJoyY - 75);
                if (absX > absY) {
                    let localSpeed = Math.round(absX / 10);
                    if (localJoyX > 90 && (oldValue != "r" || localSpeed != oldSpeed)) {
                        ws.send(`r${localSpeed}`);
                        oldValue = "r";
                        oldSpeed = localSpeed;
                    }
                    if (localJoyX < 60 && (oldValue != "l" || localSpeed != oldSpeed)) {
                        ws.send(`l${localSpeed}`);
                        oldValue = "l";
                        oldSpeed = localSpeed;
                    }
                } else {
                    let localSpeed = Math.round(absY / 10);
                    if (localJoyY > 90 && (oldValue != "d" || localSpeed != oldSpeed)) {
                        ws.send(`d${localSpeed}`);
                        oldValue = "d";
                        oldSpeed = localSpeed;
                    }
                    if (localJoyY < 60 && (oldValue != "u" || localSpeed != oldSpeed)) {
                        ws.send(`u${localSpeed}`);
                        oldValue = "u";
                        oldSpeed = localSpeed;
                    }
                }
                send = true;
            } else {
                if (send) {
                    oldValue = "stopJoy";
                    ws.send("stopJoy");
                    send = false;
                }
            }
        } else {
            text = textTemplate.error;
        }
    }, 150);
}

window.onload = () => {
    var canvas = document.getElementById('canvas');
    var ctx = canvas.getContext('2d');
    render();

    function render() {
        if (timeout + 1000 < Date.now()) { clear() }
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        for (let i = 0; i < 64; i++) {
            for (let j = 0; j < 100; j++) {
                if (mapArray[i] && mapArray[i][j]) {
                    let w = i * 8;
                    let h = j * 8
                    ctx.fillStyle = '#7188d9'
                    ctx.fillRect(h, w, h + 8, w + 8);
                } else {
                    let w = i * 8;
                    let h = j * 8
                    ctx.fillStyle = '#1d1d1d'
                    ctx.fillRect(h, w, h + 8, w + 8);
                }
            }
        }
        ctx.fillStyle = "orange";
        ctx.font = "18px cursive";
        ctx.fillText(`Информация: ${text}`, 10, 25)
    }
    setInterval(render, 500);
    socket();
    joystickRender();
}