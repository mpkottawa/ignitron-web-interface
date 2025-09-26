String json = "{";
json += "\"line0\":\"" + String(line0) + "\",";
json += "\"line1\":\"" + String(line1) + "\",";
json += "\"cursor\":" + String(cursor) + ",";
json += "\"bt\":" + String(ic.bluetooth) + ",";
json += "\"bat\":" + String(ic.battery);

if (bank >= 0) {
    json += ",\"bank\":" + String(bank);
}
if (preset >= 0) {
    json += ",\"preset\":" + String(preset);
}

json += "}";
