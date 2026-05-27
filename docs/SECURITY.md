# Security Model

This build follows a **High Entropy Security** model to ensure your configurations and secrets remain private.

## 1. Secret Isolation
Never hardcode API keys in scripts. This build uses isolated `.env` files stored in `~/.config/`:

### Weather Secret (`~/.config/dwm_weather.env`)
```bash
WEATHER_API_KEY="your_key"
WEATHER_LAT="your_lat"
WEATHER_LON="your_lon"
```

### Football Secret (`~/.config/dwm_football.env`)
```bash
FOOTBALL_API_KEY="your_token"
```

## 2. File Permissions
The `install.sh` script and the fetcher scripts enforce strict **Owner-Only** access:
- **Umask 077:** All new files created during installation are restricted to the owner.
- **Strict 600:** Cache files (`~/.cache/football_matches.cache`) and Log files are set to 600 permissions. This prevents other users on the system from reading your API logs or cached data.

## 3. Data Sanitization
Since the status bar pulls data from external websites (Footybite) and APIs:
- All strings are passed through a sanitization layer.
- Non-printable characters and terminal control codes are stripped.
- This prevents "Terminal Injection" attacks where a malicious site could try to execute commands via the status bar.

## 4. No Display Manager
By avoiding SDDM/LightDM and using a physical TTY login, you reduce the attack surface of your system and maintain complete control over your graphical session.
