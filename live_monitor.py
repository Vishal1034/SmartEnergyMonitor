import pandas as pd
from sklearn.tree import DecisionTreeClassifier
import requests
import time
import warnings
import os
from dotenv import load_dotenv
load_dotenv()

warnings.filterwarnings("ignore")

CHANNEL_ID = os.getenv('TS_CHANNEL_ID')
READ_API_KEY = os.getenv('TS_READ_API_KEY')

print("🧠 [1/2] Booting AI and loading historical dataset...")

df = pd.read_csv('feeds.csv')
df = df.dropna(subset=['field3', 'field5'])

def label_appliance(power):
    if power < 0.2:
        return '🔴 OFF (No Load)'
    elif power >= 0.2 and power <= 3.0: 
        return '💡 0.5W Night Bulb'
    elif power > 3.0: 
        return '💡 9W LED Bulb'
    else:
        return 'Unknown'

df['Appliance'] = df['field3'].apply(label_appliance)

X = df[['field3', 'field5']] 
y = df['Appliance']

ai_model = DecisionTreeClassifier(max_depth=5)
ai_model.fit(X, y)
print("✅ AI Model Trained & Ready!\n")

print("🚀 [2/2] Connecting to Cloud... Starting Live AI Inference!")
print("-" * 50)

last_prediction = ""

while True:
    try:
        url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?api_key={READ_API_KEY}&results=1"
        response = requests.get(url, timeout=10)
        data = response.json()
        
        if 'feeds' in data and len(data['feeds']) > 0:
            feed = data['feeds'][0]

            live_power = float(feed['field3'])
            live_pf = float(feed['field5'])
            
            prediction = ai_model.predict([[live_power, live_pf]])[0]
            
            print(f"📡 Live Sensor -> Power: {live_power:0.1f}W | PF: {live_pf:0.2f}  ==>  🤖 AI Detects: {prediction}")
            
    except Exception as e:
        print(f"⚠️ Network error waiting for cloud data... Retrying...")
        
    time.sleep(20)