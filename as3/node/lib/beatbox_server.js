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
    socket.on("action", function (data) {
        // console.log('action sent from ui: ' + data.action);
        // Info for connecting to the local process via UDP
        var PORT = 12345;
        var HOST = '127.0.0.1';

        var buffer = new Buffer(data.action);

        // everytime we send a message to C, if we don't get a response within 1 second
        // emit an error message to let front end know C is offline
        var connectionTimer = setTimeout(function () {
            socket.emit("connectionTimeOut");
        }, 1000);

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
            // clear timer once we recieve a message
            clearTimeout(connectionTimer);
            socket.emit("connectionEstablished")
            processUDPResponse(socket, reply, data)


        });

    });
}

function processUDPResponse(socket, reply) {
    var response;

    var action = reply.split("&")[0];
    console.log("Action recieved in UDP: ", action);
    var value = reply.split("&")[1]
    switch (action) {
        case "volume":
            socket.emit("volume_change", value)
            break;
        case "bpm":
            socket.emit("bpm_change", value)
            break;
        case "uptime":
            socket.emit("update_time", value)
            break;
        case "1":
            socket.emit("mode1");
            break;
        case "2":
            socket.emit("mode2");
            break;
        case "0":
            socket.emit("none");
            break;
        default:
            response = "Nothing returned from UDP"

    }
    return response
}
