#!/usr/bin/env python3
"""
footybite_scraper.py — Scrape live/upcoming matches from home.footybite.vc

Modes:
  (default)   Output filtered matches as pipe-separated labels for football_matches dedup
  --links     Output ALL matches with URLs for rofi selector, grouped by sport category

Output format (--links):
  ---CATEGORY_NAME---              (section header)
  EMOJI STATUS | Team1 vs Team2|||URL

Output format (default):
  🕒 Team vs Team (status) | ...
"""

import urllib.request
import sys
import re
from html.parser import HTMLParser


# ─── Sport category emoji mapping ───────────────────────────────────────────
CATEGORY_EMOJI = {
    "important games": "⚽",
    "f1":              "🏎️",
    "wnba":            "🏀",
    "nba":             "🏀",
    "nhl":             "🏒",
    "mlb":             "⚾",
    "nfl":             "🏈",
    "ufc":             "🥊",
    "boxing":          "🥊",
    "cricket":         "🏏",
    "tennis":          "🎾",
    "rugby":           "🏉",
    "motogp":          "🏍️",
}

# ─── Target teams for the default (filtered) mode ───────────────────────────
TARGET_TEAMS = [
    # International
    "Spain", "Argentina", "France", "England", "Brazil", "Portugal",
    "Netherlands", "Morocco", "Belgium", "Germany", "Croatia", "Senegal",
    "Italy", "Colombia", "United States", "USA", "Mexico", "Uruguay",
    "Switzerland", "Japan", "Iran",
    # Clubs
    "Real Madrid", "Bayern", "Liverpool", "Inter", "Manchester City",
    "Man City", "Paris Saint-Germain", "PSG", "Barcelona", "Barca",
    "Arsenal", "Bayer Leverkusen", "Dortmund", "Atletico", "Chelsea",
    "Roma", "Benfica", "Atalanta", "Sporting", "Frankfurt", "Tottenham",
    "Spurs", "Porto", "Manchester United", "Man United", "Club Brugge",
    "Fiorentina", "Real Betis", "Aston Villa", "Juventus", "Juve",
    "PSV", "Feyenoord", "West Ham", "Lille", "Milan", "Lyon", "Bodo",
    "Napoli", "Olympiacos", "AZ Alkmaar", "Leipzig", "Lazio", "Ajax",
    "Newcastle", "Brighton", "Wolves", "Everton", "Brentford",
    "Crystal Palace", "Fulham", "Nottingham Forest", "Nottingham",
    "Leicester", "Ipswich", "Southampton", "Bournemouth",
]


class FootybiteParser(HTMLParser):
    """
    Parse the home.footybite.vc HTML to extract matches grouped by category.

    HTML structure (per section):
      <div class="my-1">                      ← category header container
        <img ... alt="F1">  <span>F1</span>   ← non-"Important Games" categories
      </div>
      OR for Important Games:
        <h4 ...> Important Games</h4>

      <a target="_blank" href="https://footybite.vc/Team1-vs-Team2/12345">
        <span class="txt-team">Team1</span>
        <span>Starts in 8min</span>           ← middle column status
        <span class="txt-team">Team2</span>
      </a>
    """

    def __init__(self):
        super().__init__()
        # Final result: list of (category, home, away, status, url)
        self.matches = []

        # State tracking
        self._current_category = "Important Games"
        self._in_h4 = False
        self._in_category_span = False
        self._saw_category_icon = False  # True after we see an <img class="img-icone">

        self._current_url = ""
        self._in_match_link = False
        self._in_team_span = False
        self._in_status_span = False
        self._home = ""
        self._away = ""
        self._status = ""
        self._team_count = 0

        # Track nesting inside the time-txt div
        self._in_time_div = False

    def handle_starttag(self, tag, attrs):
        d = dict(attrs)

        # ── Category headers ──
        if tag == "h4":
            self._in_h4 = True

        # Category icon: <img ... class="img-icone" alt="F1">
        if tag == "img" and "img-icone" in d.get("class", ""):
            alt = d.get("alt", "").strip()
            if alt:
                self._current_category = alt
                self._saw_category_icon = True

        # Span right after category icon holds category name
        if tag == "span" and self._saw_category_icon:
            self._in_category_span = True

        # ── Match link ──
        if tag == "a" and d.get("target") == "_blank":
            href = d.get("href", "")
            if "footybite.vc/" in href and "-vs-" in href.lower():
                self._current_url = href
                self._in_match_link = True
                self._team_count = 0
                self._home = ""
                self._away = ""
                self._status = ""

        # ── Team name ──
        if tag == "span" and d.get("class", "") == "txt-team" and self._in_match_link:
            self._in_team_span = True

        # ── Status / time (middle column) ──
        if tag == "div" and "time-txt" in d.get("class", "") and self._in_match_link:
            self._in_time_div = True

        # The actual status text is inside a nested <span> within time-txt
        if tag == "span" and self._in_time_div and self._in_match_link:
            self._in_status_span = True

    def handle_endtag(self, tag):
        if tag == "h4":
            self._in_h4 = False
        if tag == "div" and self._in_time_div:
            self._in_time_div = False
            self._in_status_span = False
        if tag == "a" and self._in_match_link:
            # Finalize match
            if self._home and self._away and self._current_url:
                self.matches.append((
                    self._current_category,
                    self._home,
                    self._away,
                    self._status.strip() or "Scheduled",
                    self._current_url,
                ))
            self._in_match_link = False

    def handle_data(self, data):
        text = data.strip()
        if not text:
            return

        # Category from <h4>
        if self._in_h4:
            if text.lower().startswith("important"):
                self._current_category = "Important Games"

        # Category from <span> after icon
        if self._in_category_span:
            self._current_category = text
            self._in_category_span = False
            self._saw_category_icon = False

        # Team names
        if self._in_team_span and self._in_match_link:
            if self._team_count == 0:
                self._home = text
            else:
                self._away = text
            self._team_count += 1
            self._in_team_span = False

        # Status
        if self._in_status_span and self._in_match_link:
            # Skip "Live Streams" button text or empty fragments
            clean = text.strip()
            if clean and clean.lower() != "live streams":
                if not self._status:
                    self._status = clean


def status_emoji(status_text):
    """Return an appropriate emoji for match status."""
    s = status_text.lower()
    if "started" in s or "live" in s:
        return "🔴"
    elif "starts in" in s:
        return "🕒"
    elif "ended" in s or "finished" in s:
        return "⏹️"
    return "📅"


def is_target_match(home, away):
    """Check if both teams are in the target list (for default/filtered mode)."""
    h_lower = home.lower()
    a_lower = away.lower()
    home_ok = any(t.lower() in h_lower for t in TARGET_TEAMS)
    away_ok = any(t.lower() in a_lower for t in TARGET_TEAMS)
    return home_ok and away_ok


def fetch_page():
    """Fetch and return the HTML from home.footybite.vc."""
    headers = {
        "User-Agent": (
            "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"
        ),
    }
    req = urllib.request.Request("https://home.footybite.vc", headers=headers)
    with urllib.request.urlopen(req, timeout=15) as resp:
        return resp.read().decode("utf-8", errors="replace")


def main():
    show_links = "--links" in sys.argv

    try:
        html = fetch_page()
    except Exception as e:
        print(f"Error fetching footybite.vc: {e}", file=sys.stderr)
        sys.exit(1)

    parser = FootybiteParser()
    parser.feed(html)

    if not parser.matches:
        sys.exit(0)

    if show_links:
        # ── rofi mode: all matches, grouped by category ──
        current_cat = None
        for category, home, away, status, url in parser.matches:
            if category != current_cat:
                current_cat = category
                cat_emoji = CATEGORY_EMOJI.get(category.lower(), "🏅")
                print(f"---{cat_emoji} {category}---")

            emoji = status_emoji(status)
            # Clean up status for display
            display_status = re.sub(r"\s+", " ", status).strip()
            print(f"{emoji} {display_status} | {home} vs {away}|||{url}")
    else:
        # ── default mode: filtered, pipe-separated labels for football_matches ──
        labels = []
        for category, home, away, status, url in parser.matches:
            # Only include Important Games / football that match target teams
            if category.lower() == "important games" and is_target_match(home, away):
                emoji = status_emoji(status)
                display_status = re.sub(r"\s+", " ", status).strip()
                labels.append(f"{emoji} {home} vs {away} ({display_status})")
        if labels:
            print(" | ".join(labels))


if __name__ == "__main__":
    main()
