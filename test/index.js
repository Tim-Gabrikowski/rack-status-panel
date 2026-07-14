const mqtt = require("mqtt");

const broker = "mqtt://192.168.174.22:1883";
const username = "";
const password = "";

const topic = "status/rackpanel";

const client = mqtt.connect(broker, {
  username,
  password,
});

// Data structure:
// device[4]
//   metric[4]
//     { a, b }

const devices = [
  {
    metrics: [
      { a: 50, b: 60 },
      { a: 70, b: 80 },
      { a: 20, b: 30 },
      { a: 40, b: 50 },
    ],
  },
  {
    metrics: [
      { a: 60, b: 70 },
      { a: 80, b: 90 },
      { a: 10, b: 20 },
      { a: 30, b: 40 },
    ],
  },
  {
    metrics: [
      { a: 50, b: 60 },
      { a: 70, b: 80 },
      { a: 90, b: 100 },
      { a: 10, b: 20 },
    ],
  },
  {
    metrics: [
      { a: 30, b: 40 },
      { a: 50, b: 60 },
      { a: 70, b: 80 },
      { a: 90, b: 100 },
    ],
  },
];

function encodeStatus(devices) {
  const buffer = [];

  for (const device of devices) {
    for (const metric of device.metrics) {
      buffer.push(Math.round(Math.random() * 100));
      buffer.push(Math.round(Math.random() * 100));
    }
  }

  return Buffer.from(buffer);
}

client.on("connect", () => {
  setInterval(() => {
    const payload = encodeStatus(devices);
    console.log("Sending:", [...payload]);

    client.publish(topic, payload, { retain: true }, (err) => {
      if (err) {
        console.error(err);
      } else {
        console.log(`Published ${payload.length} bytes`);
      }

      //   client.end();
    });
  }, 1500);
});
