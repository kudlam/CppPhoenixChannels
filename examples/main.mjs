import {Socket} from "./phoenix.mjs"
console.log("Starting phoenix channels test")
let socket = new Socket("/socket", {params: {userToken: "123"}})
socket.connect()
let channel = socket.channel("room:123", {token: roomToken})
channel.on("new_msg", msg => console.log("Got message", msg) )
$input.onEnter( e => {
  channel.push("new_msg", {body: e.target.val}, 10000)
    .receive("ok", (msg) => console.log("created message", msg) )
    .receive("error", (reasons) => console.log("create failed", reasons) )
    .receive("timeout", () => console.log("Networking issue...") )
})

channel.join()
  .receive("ok", ({messages}) => console.log("catching up", messages) )
  .receive("error", ({reason}) => console.log("failed join", reason) )
  .receive("timeout", () => console.log("Networking issue. Still waiting..."))
channel.push("new_msg", {body: chatInput.value})
