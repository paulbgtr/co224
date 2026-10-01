import express, { type Request, type Response } from "express";
import { connect } from "mqtt";

const PORT = Number(process.env.PORT ?? 3000);
const MQTT_URL = process.env.MQTT_URL ?? "mqtt://localhost:1883";

type DeviceState = {
  co2?: number;
  t?: number;
  rh?: number;
  measuredAt?: string;
  receivedAt?: string;
  status: "online" | "offline" | "unknown";
};

const devices = new Map<string, DeviceState>();
const DEVICE_ID_RE = /^co2-[0-9a-f]{6}$/;

const client = connect(MQTT_URL, {
  username: process.env.MQTT_USERNAME,
  password: process.env.MQTT_PASSWORD,
});

client.on("connect", () => {
  console.log("Connected to MQTT broker");
  client.subscribe(["co2/+/data", "co2/+/status"], { qos: 1 }, (err) => {
    if (err) console.error("Subscribe failed:", err);
  });
});

client.on("error", (err) => console.error("MQTT error:", err));

const isMeasurement = (
  x: any,
): x is { co2: number; t: number; rh: number; ts: number } => {
  return (
    typeof x === "object" &&
    x !== null &&
    typeof x.co2 === "number" &&
    typeof x.t === "number" &&
    typeof x.rh === "number" &&
    typeof x.ts === "number"
  );
};

client.on("message", (topic, payload) => {
  const [, deviceId, kind] = topic.split("/");
  if (!deviceId || !DEVICE_ID_RE.test(deviceId)) return;

  const state = devices.get(deviceId) ?? { status: "unknown" };

  if (kind === "status") {
    const s = payload.toString();
    state.status = s === "online" ? "online" : "offline";
  } else if (kind === "data") {
    let data: unknown;
    try {
      data = JSON.parse(payload.toString());
    } catch {
      console.warn(`Invalid JSON from ${deviceId}`);
      return;
    }
    if (!isMeasurement(data)) {
      console.warn(`Unexpected payload from ${deviceId}`);
      return;
    }
    state.co2 = data.co2;
    state.t = data.t;
    state.rh = data.rh;
    state.measuredAt =
      data.ts > 0 ? new Date(data.ts * 1000).toISOString() : undefined;
    state.receivedAt = new Date().toISOString();
  }

  devices.set(deviceId, state);
});

const app = express();

app.get("/api/devices", (_req: Request, res: Response) => {
  res.json({ success: true, data: Object.fromEntries(devices) });
});

app.get("/api/devices/:deviceId", (req: Request, res: Response) => {
  const data = devices.get(req.params.deviceId as string);
  if (!data) {
    res.status(404).json({ success: false, error: "Device not found" });
    return;
  }
  res.json({ success: true, data });
});

app.listen(PORT, () =>
  console.log(`Backend running on http://localhost:${PORT}`),
);
