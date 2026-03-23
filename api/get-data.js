// api/get-data.js
export default async function handler(req, res) {
    // 1. Pull the hidden keys from Vercel's secure vault
    const channelId = process.env.TS_CHANNEL_ID;
    const readApiKey = process.env.TS_READ_API_KEY;

    // 2. Prevent CORS errors so your frontend can read the data safely
    res.setHeader('Access-Control-Allow-Credentials', true);
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET,OPTIONS');

    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // 3. Fetch the actual data from ThingSpeak
    try {
        const tsUrl = `https://api.thingspeak.com/channels/${channelId}/feeds.json?api_key=${readApiKey}&results=1`;
        const response = await fetch(tsUrl);
        
        if (!response.ok) {
            throw new Error(`ThingSpeak API responded with status: ${response.status}`);
        }

        const data = await response.json();
        
        // 4. Send the data back to your dashboard!
        res.status(200).json(data);
    } catch (error) {
        console.error("Vercel Backend Error:", error);
        res.status(500).json({ error: "Failed to fetch data from ThingSpeak" });
    }
}