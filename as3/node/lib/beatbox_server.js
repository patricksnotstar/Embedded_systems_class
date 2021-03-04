/*
 * Respond to commands over a websocket to do interact with BBG
 */

var socketio = require('socket.io');
var io;
var dgram = require('dgram');

exports.listen = function (server) {
    io = socketio.listen(server);
    io.set('log level 1');

    io.sockets.on('connection', function (socket) {

        handleCommand(socket);
    });
};


function handleCommand(socket) {
    // var errorTimer = setTimeout(function () {
    //     socket.emit("daError", "Oops: User too slow at sending first command.");
    // }, 5000);

    // Stop the timer:
    // clearTimeout(errorTimer);


    socket.on("action", function (data) {
        // console.log('action sent from ui: ' + data.action);
        // Info for connecting to the local process via UDP
        var PORT = 12345;
        var HOST = '127.0.0.1';

        // need to store different info in buffer for each socket.on function below and then create packet afterwards ?
        var buffer = new Buffer(data.action);
        // console.log("Buffer created in server: ", buffer);

        var client = dgram.createSocket('udp4');
        client.send(buffer, 0, buffer.length, PORT, HOST, function (err, bytes) {
            if (err)
                throw err;
            console.log('UDP message sent to ' + HOST + ':' + PORT);
        });

        client.on('listening', function () {
            var address = client.address();
            console.log('UDP Client: listening on ' + address.address + ":" + address.port);
        });
        // Handle an incoming message over the UDP from the local application.
        client.on('message', function (message, remote) {
            console.log("UDP Client: message Rx" + remote.address + ':' + remote.port + ' - ' + message);

            var reply = message.toString('utf8')

            processUDPResponse(socket, reply, data)
            // socket.emit('reply', reply);

            // client.close();

        });

    });
    // socket.on("changeMode", function (data) {
    //     var mode = data.mode;

    //     // console.log('mode changed to: ' + mode);
    // });

    // socket.on("changeVolume", function (data) {

    //     var vol = Number(data.currentVol);

    //     if (data.action === "up") {
    //         if (vol < 100) {
    //             socket.emit("volume", vol + 5);
    //             // send packet to host and actually increase volume

    //         }
    //         else {
    //             socket.emit("volume", vol);
    //         }
    //         console.log("increasing volume");
    //     }
    //     else {
    //         if (data.action === "down") {
    //             if (vol > 0) {
    //                 socket.emit("volume", vol - 5);
    //                 // send packet to host and actually decrease volume

    //             }
    //             else {
    //                 socket.emit("volume", vol);
    //             }
    //         }
    //     }
    //     // console.log('mode changed to: ' + mode);
    // });

    // socket.on("changeBPM", function (data) {

    //     var bpm = Number(data.currentBPM);

    //     if (data.action === "up") {
    //         if (bpm < 300) {
    //             socket.emit("BPM", bpm + 5);
    //         }
    //         else {
    //             socket.emit("BPM", bpm);
    //         }
    //         console.log("increasing volume");
    //     }
    //     else {
    //         if (data.action === "down") {
    //             if (bpm > 40) {
    //                 socket.emit("BPM", bpm - 5);
    //             }
    //             else {
    //                 socket.emit("BPM", bpm);
    //             }
    //         }
    //     }
    //     // console.log('mode changed to: ' + mode);
    // });

    // socket.on("playDrums", function (data) {

    //     // need to hook up to UDP to trigger actual playback

    //     // console.log('Playing drum sound: ' + data.drum);
    // });


    // client.on("UDP Client: close", function () {
    //     console.log("closed");
    // });
    // client.on("UDP Client: error", function (err) {
    //     console.log("error: ", err);
    // });
}

function processUDPResponse(socket, reply, data) {
    var response;
    switch (data.action) {
        case "volume_up":
            socket.emit("volume_change", reply)
        case "volume_down":
            socket.emit("volume_change", reply)
        case "bpm_up":
            socket.emit("bpm_up", reply)
        case "bpm_down":
            socket.emit("bpm_down", reply)
        case "uptime":
            socket.emit("update_time", reply)
        default:
            response = "Nothing returned from UDP"

    }
    return response
}
