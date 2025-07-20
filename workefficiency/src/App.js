import React, { useState } from "react";

function secondsToHMS(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return `${h} שעות, ${m} דקות, ${s} שניות`;
}

function App() {
  const [message, setMessage] = useState("לחץ על הכפתור והנח את האצבע על החיישן");
  const [climate, setClimate] = useState("");
  const [workTime, setWorkTime] = useState(null);
  const [status, setStatus] = useState("");

  const MAX_WORK_SECONDS = 8 * 3600; // 8 שעות עבודה

  const handleScan = () => {
    setMessage("⏳ מחכה לטביעת אצבע...");
    // fetch("http://192.168.22.54/scan")
     fetch("http://10.0.0.10/scan")
      .then((res) => res.json())
      .then((data) => {
        if (data.status === "error") {
          setMessage("❌ טביעת אצבע לא מזוהה, נסה שוב.");
          setWorkTime(null);
          setClimate("");
          setStatus("");
          return;
        }

        setStatus(data.status);

        // חישוב זמן עבודה יחסית למקסימום
        setWorkTime(`${secondsToHMS(data.totalWorkSeconds)} מתוך 8 שעות`);

        // המלצות אקלים
        const temp = data.temperature;
        const humidity = data.humidity;
        let climateSuggestion = "";

        if (temp === 0 && humidity === 0) {
          climateSuggestion = "❗ חיישן טמפרטורה לא מחובר או תקול.";
        } else if (temp < 20) {
          climateSuggestion = `🥶 קר – כדאי להעלות את המזגן`;
        } else if (temp >= 20 && temp <= 26) {
          climateSuggestion = `😌 נעים – אין צורך לשנות את המזגן`;
        } else if (temp > 26) {
          climateSuggestion = `🥵 חם – מומלץ להדליק מזגן`;
        }

        setClimate(`🌡️ טמפרטורה: ${temp}°C | 💧 לחות: ${humidity}%\n${climateSuggestion}`);

        // הודעות נוספות
        if (!data.isPresent) {
          setMessage(`⚠️ עובד ${data.id} מזוהה, אך לא יושב מול העמדה.`);
        } else if (!data.isActive) {
          setMessage(`😴 עובד ${data.id} נמצא אך אינו פעיל. ⏰ נשלח צליל תזכורת!`);
        } else {
          setMessage(`✅ עובד ${data.id} פעיל בעבודה.`);
        }
      })
      .catch(() => {
        setMessage("⚠️ שגיאה בחיבור ל־ESP32. ודא שהוא מחובר לאותה רשת.");
        setClimate("");
        setWorkTime(null);
        setStatus("");
      });
  };

  return (
    <div style={{ maxWidth: 480, margin: "40px auto", padding: 20, fontFamily: "Arial, sans-serif", direction: "rtl", backgroundColor: "#f0f4f8", borderRadius: 10, boxShadow: "0 4px 10px rgba(0,0,0,0.1)" }}>
      <h1 style={{ textAlign: "center", color: "#004d99" }}>📋 מערכת נוכחות חכמה</h1>
      <button
        onClick={handleScan}
        style={{
          width: "100%",
          padding: "15px",
          fontSize: "20px",
          backgroundColor: "#007acc",
          color: "white",
          border: "none",
          borderRadius: 8,
          cursor: "pointer",
          marginBottom: 20,
          transition: "background-color 0.3s",
        }}
        onMouseEnter={(e) => (e.target.style.backgroundColor = "#005f99")}
        onMouseLeave={(e) => (e.target.style.backgroundColor = "#007acc")}
      >
        🔍 סרוק טביעת אצבע
      </button>

      <div style={{ backgroundColor: "white", padding: 20, borderRadius: 8, minHeight: 140, boxShadow: "inset 0 0 10px #ccc" }}>
        <p style={{ fontSize: 18, minHeight: 50 }}>{message}</p>
        {status && <p style={{ color: "#006600", fontWeight: "bold" }}>{status}</p>}
        {workTime && (
          <p style={{ marginTop: 10, fontSize: 16 }}>
            ⏰ זמן עבודה: <span style={{ fontWeight: "bold" }}>{workTime}</span>
          </p>
        )}
        {climate && (
          <pre style={{ whiteSpace: "pre-wrap", fontSize: 16, color: "#444", marginTop: 10 }}>{climate}</pre>
        )}
      </div>
    </div>
  );
}

export default App;
