// Set up: npm install
var http = require("http"),
    fs = require("fs"),
    path = require("path"),
    ws = require("ws"),
    open = require("open"),
    ProtoBuf = require("protobufjs");

// Copy dependencies to "www/" (example specific, you usually don't have to care
var deps = [
    ["Long.min.js", "./node_modules/protobufjs/node_modules/bytebuffer/node_modules/long/Long.min.js"],
    ["ByteBuffer.min.js", "./node_modules/protobufjs/node_modules/bytebuffer/ByteBuffer.min.js"],
    ["ProtoBuf.min.js", "./node_modules/protobufjs/ProtoBuf.min.js"]
];
for (var i=0, dep, data; i<deps.length; i++) {
    dep = deps[i];
    if (!fs.existsSync(path.join(__dirname, "www", dep[0]))) {
        console.log("Copying "+dep[0]+" from "+dep[1]);
        try {
            fs.writeFileSync(path.join(__dirname, "www", dep[0]), fs.readFileSync(path.join(__dirname, dep[1])));
        } catch (err) {
            console.log("Copying failed: "+err.message);
            console.log("\nDid you run `npm install` ?");
            process.exit(1);
        }
    }
}

// Initialize from .proto file
var builder = ProtoBuf.loadProtoFile(path.join(__dirname, "www", "example.proto")),
    Message = builder.build("Message");

// HTTP server
var server = http.createServer(function(req, res) {
        var file = null,
            type = "text/html";
        if (req.url == "/") {
            file = "index.html";
        } else if (/^\/(\w+(?:\.min)?\.(?:js|html|proto))$/.test(req.url)) {
            file = req.url.substring(1);
            if (/\.js$/.test(file)) {
                type = "text/javascript";
            }
        }
        if (file) {
            fs.readFile(path.join(__dirname, "www", file), function(err, data) {
                if (err) {
                    res.writeHead(500, {"Content-Type": type});
                    res.end("Internal Server Error: "+err);
                } else {
                    res.writeHead(200, {"Content-Type": type});
                    res.write(data);
                    res.end();
                    console.log("Served www/"+file);
                }
            });
        } else {
            res.writeHead(404, {"Content-Type": "text/html"});
            res.end("Not Found");
        }
    });
server.listen(8080);
server.on("listening", function() {
    console.log("Server started");
    open("http://localhost:8080/");
});
server.on("error", function(err) {
    console.log("Failed to start server:", err);
    process.exit(1);
});
    
// WebSocket adapter
var wss = new ws.Server({server: server});
wss.on("connection", function(socket) {
    console.log("New WebSocket connection");
    socket.on("close", function() {
        console.log("WebSocket disconnected");
    });
    socket.on("message", function(data, flags) {
        if (flags.binary) {
            try {
                // Decode the Message
                var msg = Message.decode(data);
                console.log("Received: "+msg.text);
                // Transform the text to upper case
                msg.text = msg.text.toUpperCase();
                // Re-encode it and send it back
                socket.send(msg.toBuffer());
                console.log("Sent: "+msg.text);
            } catch (err) {
                console.log("Processing failed:", err);
            }
        } else {
            console.log("Not binary data");
        }
    });
});
