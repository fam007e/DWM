import urllib.request
import re
from html.parser import HTMLParser
from datetime import datetime, timezone
import concurrent.futures

# Helper to convert UK time to Local time
def uk_to_local(date_str, time_str):
    try:
        dt_str = f"{date_str} {time_str}"
        dt_utc = datetime.strptime(dt_str, "%Y-%m-%d %H:%M").replace(tzinfo=timezone.utc)
        local_dt = dt_utc.astimezone()
        return local_dt.strftime("%H:%M")
    except:
        return time_str

class FootybiteParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.matches = []
        self.current_match = {}
        self.in_team = False
        self.team_count = 0
        # === YOUR TOP TEAMS & CLUBS LIST ===
        self.target_teams = [
            # International
            "Spain", "Argentina", "France", "England", "Brazil", "Portugal", "Netherlands", 
            "Morocco", "Belgium", "Germany", "Croatia", "Senegal", "Italy", "Colombia", 
            "United States", "USA", "Mexico", "Uruguay", "Switzerland", "Japan", "Iran",
            # Clubs
            "Real Madrid", "Bayern", "Liverpool", "Inter", "Manchester City", "Man City",
            "Paris Saint-Germain", "PSG", "Barcelona", "Barca", "Arsenal", "Bayer Leverkusen",
            "Dortmund", "Atletico", "Chelsea", "Roma", "Benfica", "Atalanta", "Sporting",
            "Frankfurt", "Tottenham", "Spurs", "Porto", "Manchester United", "Man United",
            "Club Brugge", "Fiorentina", "Real Betis", "Aston Villa", "Juventus", "Juve",
            "PSV", "Feyenoord", "West Ham", "Lille", "Milan", "Lyon", "Bodo", "Napoli",
            "Olympiacos", "AZ Alkmaar", "Leipzig", "Lazio", "Ajax",
            "Newcastle", "Brighton", "Wolves", "Everton", "Brentford", "Crystal Palace", 
            "Fulham", "Nottingham Forest", "Nottingham", "Leicester", "Ipswich", 
            "Southampton", "Bournemouth"
        ]

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        if tag == "a" and "href" in attrs_dict:
            if "-vs-" in attrs_dict["href"]:
                self.current_match["url"] = attrs_dict["href"]
        elif tag == "span" and attrs_dict.get("class") == "txt-team":
            self.in_team = True

    def handle_data(self, data):
        data = data.strip()
        if self.in_team and data:
            if self.team_count == 0:
                self.current_match["home"] = data
                self.team_count = 1
            else:
                self.current_match["away"] = data
                self.add_match()
            self.in_team = False

    def add_match(self):
        home = self.current_match.get("home", "")
        away = self.current_match.get("away", "")
        url = self.current_match.get("url", "")
        
        # Check if BOTH teams are in our target list
        home_is_target = any(target.lower() in home.lower() for target in self.target_teams)
        away_is_target = any(target.lower() in away.lower() for target in self.target_teams)
        
        if home_is_target and away_is_target:
            self.matches.append({"home": home, "away": away, "url": url})
        
        self.current_match = {}
        self.team_count = 0

def fetch_match_detail(m):
    url = m["url"]
    try:
        if not url.startswith("http"):
            url = "https://www.footybite.do" + (url if url.startswith("/") else "/" + url)
        
        headers = {'User-Agent': 'Mozilla/5.0'}
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req, timeout=5) as response:
            html = response.read().decode('utf-8')
            match = re.search(r"(\d{4}-\d{2}-\d{2}),\s+(\d{2}:\d{2})\s+uk time", html)
            if match:
                dt_str = f"{match.group(1)} {match.group(2)}"
                dt_utc = datetime.strptime(dt_str, "%Y-%m-%d %H:%M").replace(tzinfo=timezone.utc)
                now_utc = datetime.now(timezone.utc)
                diff = (dt_utc - now_utc).total_seconds()
                
                # Filter: starts within 6 hours (21600s) OR started up to 3 hours ago (10800s)
                if -10800 < diff < 21600:
                    local_time = uk_to_local(match.group(1), match.group(2))
                    return f"🕒 {m['home']} vs {m['away']} ({local_time})|||{url}"
    except:
        pass
    return None

def main():
    import sys
    show_links = "--links" in sys.argv
    try:
        headers = {'User-Agent': 'Mozilla/5.0'}
        req = urllib.request.Request("https://www.footybite.do", headers=headers)
        with urllib.request.urlopen(req, timeout=10) as response:
            html = response.read().decode('utf-8')
            parser = FootybiteParser()
            parser.feed(html)
            
            with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
                results = list(executor.map(fetch_match_detail, parser.matches))
            
            cleaned_results = [r for r in results if r]
            if show_links:
                for r in cleaned_results:
                    print(r)
            else:
                # Compatibility mode: just labels separated by |
                labels = [r.split("|||")[0] for r in cleaned_results]
                print(" | ".join(labels))
    except Exception:
        pass

if __name__ == "__main__":
    main()
