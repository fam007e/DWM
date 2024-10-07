from bs4 import BeautifulSoup
import json
from datetime import datetime, timedelta
import os
import requests
from urllib.parse import urljoin

# Default UTC offset from the website HTML
default_utc_offset = 4

# Custom UTC offset
custom_utc_offset = 1

url = "https://vipstand.pm/sports/watch-football-live"
response = requests.get(url)

# Define the directory for saving files
base_dir = os.path.expanduser('~/DWM/scripts/FootballFixtures')
if not os.path.exists(base_dir):
    os.makedirs(base_dir)

if response.status_code == 200:
    # Paths to the temporary files
    html_file_path = os.path.join(base_dir, 'football_page.html')
    txt_file_path = os.path.join(base_dir, 'football_page.txt')

    with open(html_file_path, 'w') as file:
        file.write(response.text)

    with open(html_file_path, 'r') as file:
        soup = BeautifulSoup(file, 'html.parser')
        formatted_html = soup.prettify()

    with open(txt_file_path, 'w') as txt_file:
        txt_file.write(formatted_html)

    with open(txt_file_path, 'r') as txt_file:
        soup_txt = BeautifulSoup(txt_file, 'html.parser')
        fixtures = soup_txt.find_all('a', title=True)

        fixture_data = []
        for fixture in fixtures:
            match_time_elem = fixture.find('span', attrs={'content': True})
            if match_time_elem:
                match_time = match_time_elem.get('content')
                match_date = datetime.strptime(match_time, "%Y-%m-%dT%H:%M").date().strftime("%d-%m-%y")
                match_time_adjusted = (datetime.strptime(match_time, "%Y-%m-%dT%H:%M") + timedelta(hours=default_utc_offset + int(custom_utc_offset))).strftime("%I:%M %p")
                match_info = fixture['title']
                home_team, away_team = match_info.split(' v ')
                competition = fixture.find('span', class_='vip-stand').get('class')[3].replace('-', ' ').upper()

                stream_link = fixture['href']
                full_stream_link = urljoin(url, stream_link) + "/1/"

                fixture_info = {
                    "home_team": home_team,
                    "away_team": away_team,
                    "match_time": match_time_adjusted,
                    "competition": competition,
                    "stream_link": full_stream_link
                }
                # Check if the fixture is not already in the list before appending
                if fixture_info not in fixture_data:
                    fixture_data.append(fixture_info)

        # Sort fixture_data based on match time
        fixture_data = sorted(fixture_data, key=lambda x: datetime.strptime(x['match_time'], "%I:%M %p"))

        matches_file = os.path.join(base_dir, f"matches_Date_{match_date}.json")

        # Remove duplicates from the fixture_data list
        fixture_data = [dict(t) for t in {tuple(d.items()) for d in fixture_data}]

        with open(matches_file, 'w') as outfile:
            json.dump(fixture_data, outfile, indent=4)

    # Remove the football_page.html and football_page.txt files
    os.remove(html_file_path)
    os.remove(txt_file_path)

else:
    print(f"Failed to retrieve content from {url}. Status code: {response.status_code}")

