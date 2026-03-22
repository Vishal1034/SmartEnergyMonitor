import pandas as pd
from sklearn.tree import DecisionTreeClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score
import warnings

warnings.filterwarnings("ignore")

print("🧠 Loading Master Dataset...")

try:
    df = pd.read_csv('feeds.csv')
except FileNotFoundError:
    print("❌ ERROR: Could not find 'feeds.csv'. Make sure it is in the same folder as this script!")
    exit()
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

print(f"📊 Processed {len(df)} total data points.")

X = df[['field3', 'field5']]  
y = df['Appliance']           

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print("⚙️ Training the Decision Tree Model...")
ai_model = DecisionTreeClassifier(max_depth=5)
ai_model.fit(X_train, y_train)

predictions = ai_model.predict(X_test)
accuracy = accuracy_score(y_test, predictions)
print(f"✅ AI Training Complete! Accuracy: {accuracy * 100:.2f}%\n")

print("--- 🔮 LIVE SIMULATION TEST ---")
test_signatures = [
    [0.0, 0.00],  
    [0.6, 0.65],  
    [7.5, 0.94]   
]

for sig in test_signatures:
    power, pf = sig[0], sig[1]
    guess = ai_model.predict([[power, pf]])[0]
    print(f"Reading: {power}W  |  PF: {pf}   ->   AI Predicts: {guess}")