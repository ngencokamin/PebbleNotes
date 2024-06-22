from secret import client_id, client_secret, redis_ip, redis_pass, secret_key
import config
from flask import Flask, request, jsonify, redirect, url_for, session, render_template
import random
import redis
import requests
import json
import urllib
from config import auth_redir_uri

app = Flask(__name__)
app.secret_key = secret_key

# Configure Redis
redis_client = redis.StrictRedis(host=redis_ip, port=6379,password=redis_pass, db=0)

# These words are used to generate word-based passcode.
WORDS = (
    'red green blue white brown violet purple black yellow orange '
    'dog cat cow unicorn animal hedgehog chicken '
    'task computer phone watch android robot apple '
    'rambler rogue warrior king '
    'jeans muffin cake bake cookie oven bread '
).split()

def query_json(url, data):
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    if not isinstance(data, str):
        data = urllib.parse.urlencode(data).encode('utf-8')
    try:
        response = requests.post(url, headers=headers, data=data)
        response.raise_for_status()
        return json.loads(response.content.decode('utf-8'))
    except requests.exceptions.RequestException as e:
        raise e

@app.route('/auth')
def auth_redirect():
    url = 'https://accounts.google.com/o/oauth2/auth?' + urllib.parse.urlencode(dict(
        client_id=client_id,
        redirect_uri=auth_redir_uri,
        response_type='code',
        scope='https://www.googleapis.com/auth/tasks',
        state='',
        access_type='offline',
        approval_prompt='force',
        include_granted_scopes='true',
    ))
    return redirect(url)

def json_compactify(data):
    return json.dumps(data, separators=(',', ':'))

def fetch_tokens(code):
    q = {
        "code": code,
        "client_id": client_id,
        "client_secret": client_secret,
        "redirect_uri": auth_redir_uri,
        "grant_type": "authorization_code",
    }
    result = query_json("https://accounts.google.com/o/oauth2/token", q)
    if 'access_token' not in result:
        return {'error': 'Failed to retrieve tokens'}
    return result

def generate_passcode():
    return ' '.join(random.sample(WORDS, 4))

@app.route('/auth/callback')
def auth_callback():
    code = request.args.get("code")
    if not code:
        return jsonify(error='Missing authorization code')

    tokens = fetch_tokens(code)
    if 'error' in tokens:
        return jsonify(error=tokens['error'])

    passcode = generate_passcode()
    redis_client.setex(passcode, 600, json.dumps(tokens))  # Cache passcode with tokens for 10 minutes

    return render_template('intermediate.html', passcode=passcode)

@app.route('/auth/check', methods=['POST'])
def auth_check():
    data = request.get_json()
    if not data or 'passcode' not in data:
        return jsonify(error='Missing passcode'), 400

    passcode = data['passcode']
    tokens = redis_client.get(passcode)

    if not tokens:
        return jsonify(error='Invalid or expired passcode'), 401

    tokens = json.loads(tokens)
    return jsonify(tokens)

class AuthRefresh:
    @staticmethod
    def refresh(refresh_token):
        q = {
            "refresh_token": refresh_token,
            "client_id": client_id,
            "client_secret": client_secret,
            "grant_type": "refresh_token",
        }
        result = query_json("https://accounts.google.com/o/oauth2/token", q)
        if 'access_token' not in result:
            return None
        return result

@app.route('/refresh_token', methods=['POST'])
def refresh_access_token():
    data = request.get_json()
    if not data or 'refresh_token' not in data:
        return jsonify(error='Missing refresh token'), 400

    refresh_token = data['refresh_token']
    new_tokens = AuthRefresh.refresh(refresh_token)

    if new_tokens:
        return jsonify(new_tokens)
    else:
        return jsonify(error='Failed to refresh access token'), 401

if __name__ == '__main__':
    app.run(debug=True)