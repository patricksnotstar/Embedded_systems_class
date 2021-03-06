"use strict";
// Client-side interactions with the browser.

// Web sockets: automatically establish a socket with the server
var socket = io.connect();

// Make connection to server when web page is fully loaded.
$(document).ready(function () {
    // Make the text-entry box have keyboard focus.
    $('#send-command').focus();
    $('#error-C').hide();
    $('#error-node').hide();



    // get the current volume and bpm of the board to display when webpage loads
    sendMessage("volume_get", null);
    sendMessage("bpm_get", null);

    // display error if node js server is not running
    socket.on('disconnect', function () {
        $('#error-node').show();
    })
    socket.on('connect', function () {
        $('#error-node').hide();
    })

    // display error if C code doesnt respond to message
    socket.on('connectionTimeOut', function () {
        $('#error-C').show();
    })
    // hide error if connection between C and node is fine
    socket.on('connectionEstablished', function () {
        $('#error-C').hide();
    })


    // Setup a repeating function (every 1s)
    window.setInterval(function () { sendMessage('uptime') }, 1000);


    $('#modeNone').click(function () {
        sendMessage("mode", "None");
    });
    $('#modeRock1').click(function () {
        sendMessage("mode", "Rock #1");
    });
    $('#modeRock2').click(function () {
        sendMessage("mode", "Describe");
    });
    $('#volumeUp').click(function () {
        sendMessage("volume_up", null);
    });
    $('#volumeDown').click(function () {
        sendMessage("volume_down", null);
    });
    $('#bpmUp').click(function () {
        sendMessage("bpm_up", null);
    });
    $('#bpmDown').click(function () {
        sendMessage("bpm_down", null);
    });
    $('#hi-hat').click(function () {
        sendMessage("hi-hat", null);
    });
    $('#snare').click(function () {
        sendMessage("snare", null);
    });
    $('#base').click(function () {
        sendMessage("base", null);
    });
});



socket.on("volume_change", function (result) {
    // update volume value on front end
    $("#volumeId").val(result)
});

socket.on("bpm_change", function (result) {
    // update bpm value on front end
    $("#bpmId").val(result)
});

socket.on("update_time", function (result) {
    var time = result.split(' ')[0];
    // console.log(time);
    time = new Date(Number(time) * 1000).toISOString().substr(11, 8)
    $("#statusid").text(time + " (H:M:S)")
});

socket.on('error', function (result) {
    var msg = divMessage('SERVER ERROR: ' + result);
    $('#messages').append(msg);
});



function sendMessage(action, data) {
    var message = {
        action: action,
        data: data
    }
    // console.log("Sending message to server: ", message)
    socket.emit("action", message);
}


function readUserInput() {
    // Get the user's input from the browser.
    var message = $('#send-command').val();

    // Display the command in the message list.
    $('#messages').append(divMessage(message));

    // Process the command
    var systemMessage = processCommand(message);
    if (systemMessage != false) {
        $('#messages').append(divMessage(systemMessage));
    }

    // Scroll window.
    $('#messages').scrollTop($('#messages').prop('scrollHeight'));

    // Clear the user's command (ready for next command).
    $('#send-command').val('');
}
