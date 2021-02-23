var s = require('net').Socket();

setInterval(function(){
    s.connect(3501, '192.168.188.30')//IP HERE
    s.on('data', async function(d){
        console.log(d.toString("utf-8", 4, d.readUIntBE(0, 4) + 4))
        let e = JSON.parse(d.toString("utf-8", 4, d.readUIntBE(0, 4) + 4))
    })  

    s.end()
}, 5000)