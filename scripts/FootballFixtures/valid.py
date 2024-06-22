import json

required_fields = ["home_team", "away_team", "match_time", "competition", "stream_link"]

def validate_matches(json_file):
    with open(json_file, 'r') as file:
        matches = json.load(file)

    for match in matches:
        for field in required_fields:
            if field not in match or not match[field]:
                print(f"Missing {field} in match: {match}")

# Replace with the path to your JSON file
validate_matches("/home/fam007e/dwm/scripts/FootballFixtures/matches_Date_16-06-24.json")
