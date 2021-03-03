"use strict";
// Client-side interactions with the browser.

// Web sockets: automatically establish a socket with the server
var socket = io.connect();

// Make connection to server when web page is fully loaded.
$(document).ready(function () {
    // Make the text-entry box have keyboard focus.
    $('#send-command').focus();

    // // Allow sending the form (pressing enter).
    // $('#send-form').submit(function () {
    //     readUserInput();

    //     // Return false to show we have handleded it.
    //     return false;
    // });

    // var errorTimer = setTimeout(function () {
    //     socket.emit("daError", "Oops: User too slow at sending first command.");
    // }, 5000);

    // Stop the timer:
    // clearTimeout(errorTimer);


    // Called the function in each second
    var interval = setInterval(function () {
        // TODO: get current time from /proc/uptime via UDP
        sendMessage("uptime", null);
        $("#status").val(counter)

    }, 1000); // Run for each second

    // Make this the zenCape volume later
    $("#volumeId").val(0);
    $("#bpmId").val(120);

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
        var currentVol = $("#volumeId").val();
        sendMessage("volume up", currentVol);
    });
    $('#volumeDown').click(function () {
        var currentVol = $("#volumeId").val();
        sendMessage("volume down", currentVol);
    });
    $('#bpmUp').click(function () {
        var currentBPM = $("#bpmId").val();
        sendMessage("bpm up", currentBPM);
    });
    $('#bpmDown').click(function () {
        var currentBPM = $("#bpmId").val();
        sendMessage("bpm down", currentBPM);
    });

    $('#hi-hat').click(function () {
        var sound = $('#hi-hat').val();
        sendMessage("drums", sound);
    });
    $('#snare').click(function () {
        var sound = $('#snare').val();
        sendMessage("drums", sound);
    });
    $('#base').click(function () {
        var sound = $('#base').val();
        sendMessage("drums", sound);
    });

    socket.on("reply", function (result) {
        // process reply from UDP socket and put value into front end
    });

    socket.on('error', function (result) {
        var msg = divMessage('SERVER ERROR: ' + result);
        $('#messages').append(msg);
    });

});


function sendMessage(action, data) {
    var message = {
        action: action,
        data: data
    }
    socket.emit("action", message);
}

// function changeMode(modeType) {
//     var message = {
//         mode: modeType,
//     };
//     $("#modeid").text(modeType);
//     socket.emit("changeMode", message);
//     // console.log('boobs');
// }

// function adjustVolume(direction, currentVol) {
//     var message = {
//         action: direction, //either up or down
//         currentVol: currentVol
//     };
//     socket.emit("changeVolume", message);
// }

// function adjustBPM(direction, currentBPM) {
//     var message = {
//         action: direction, //either up or down
//         currentBPM: currentBPM
//     };
//     socket.emit("changeBPM", message);
// }

// function playDrums(drumSound) {
//     var message = {
//         drum: drumSound, //either hi-hat, snare or base
//         // currentBPM: currentBPM // not sure if we need to write the BPM so it plays back at the proper speed?
//     };
//     $("#drumId").text(drumSound);
//     socket.emit("playDrums", message);
// }

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
