import random
from fastapi import FastAPI, Request, HTTPException, Depends
from fastapi.responses import RedirectResponse, HTMLResponse
from fastapi.templating import Jinja2Templates
from fastapi.middleware.trustedhost import TrustedHostMiddleware
from redis.asyncio import Redis
import aiohttp
import json
import urllib.parse
from secret import client_id, client_secret, redis_ip, redis_pass, secret_key
from config import auth_redir_uri
from datetime import datetime, timedelta
import uvicorn
from starlette.middleware.sessions import SessionMiddleware
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI()

# Configure session middleware for session handling
app.add_middleware(SessionMiddleware, secret_key=secret_key)

# Configure Trusted Host middleware for added security
app.add_middleware(TrustedHostMiddleware, allowed_hosts=["*"])

# Setup Jinja2 templates
templates = Jinja2Templates(directory="templates")

# These words are used to generate word-based passcode.
WORDS = (
    'red green blue white brown violet purple black yellow orange '
    'dog cat cow unicorn animal hedgehog chicken '
    'task computer phone watch android robot apple '
    'rambler rogue warrior king '
    'jeans muffin cake bake cookie oven bread '
).split()

async def get_redis():
    logger.info("Connecting to Redis")
    redis = Redis.from_url(f"redis://:{redis_pass}@{redis_ip}:6379/0")
    return redis

async def query_json(url, data):
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    if not isinstance(data, str):
        data = urllib.parse.urlencode(data)
    async with aiohttp.ClientSession() as session:
        logger.info(f"Sending request to {url} with data: {data}")
        async with session.post(url, headers=headers, data=data) as response:
            logger.info(f"Response status: {response.status}")
            response.raise_for_status()
            result = await response.json()
            logger.info(f"Response JSON: {result}")
            return result

@app.get('/auth')
async def auth_redirect():
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
    logger.info(f"Redirecting to Google Auth URL: {url}")
    return RedirectResponse(url)

def json_compactify(data):
    return json.dumps(data, separators=(',', ':'))

async def fetch_tokens(code):
    q = {
        "code": code,
        "client_id": client_id,
        "client_secret": client_secret,
        "redirect_uri": auth_redir_uri,
        "grant_type": "authorization_code",
    }
    logger.info(f"Fetching tokens with code: {code}")
    result = await query_json("https://accounts.google.com/o/oauth2/token", q)
    if 'access_token' not in result:
        logger.error(f"Failed to retrieve tokens: {result}")
        return {'error': 'Failed to retrieve tokens'}
    logger.info(f"Tokens retrieved: {result}")
    return result

def generate_passcode():
    passcode = ' '.join(random.sample(WORDS, 4))
    logger.info(f"Generated passcode: {passcode}")
    return passcode

@app.get('/auth/callback', response_class=HTMLResponse)
async def auth_callback(request: Request, redis: Redis = Depends(get_redis)):
    code = request.query_params.get("code")
    if not code:
        logger.error("Missing authorization code in callback request")
        raise HTTPException(status_code=400, detail="Missing authorization code")

    logger.info(f"Authorization code received: {code}")
    tokens = await fetch_tokens(code)
    if 'error' in tokens:
        logger.error(f"Error fetching tokens: {tokens['error']}")
        raise HTTPException(status_code=400, detail=tokens['error'])

    # Add token expiry time
    tokens['expires_at'] = (datetime.now() + timedelta(seconds=tokens['expires_in'])).timestamp()
    logger.info(f"Tokens with expiry added: {tokens}")

    passcode = generate_passcode()
    await redis.setex(passcode, timedelta(minutes=10), json.dumps(tokens))  # Cache passcode with tokens for 10 minutes
    logger.info(f"Stored tokens in Redis with passcode: {passcode}")

    return templates.TemplateResponse("intermediate.html", {"request": request, "passcode": passcode})

@app.post('/auth/check')
async def auth_check(data: dict, redis: Redis = Depends(get_redis)):
    if 'passcode' not in data:
        logger.error("Missing passcode in auth check request")
        raise HTTPException(status_code=400, detail="Missing passcode")

    passcode = data['passcode']
    logger.info(f"Checking tokens for passcode: {passcode}")
    tokens = await redis.get(passcode)

    if not tokens:
        logger.error("Invalid or expired passcode")
        raise HTTPException(status_code=401, detail="Invalid or expired passcode")

    tokens = json.loads(tokens)
    logger.info(f"Tokens retrieved for passcode: {tokens}")
    return tokens

class AuthRefresh:
    @staticmethod
    async def refresh(refresh_token):
        q = {
            "refresh_token": refresh_token,
            "client_id": client_id,
            "client_secret": client_secret,
            "grant_type": "refresh_token",
        }
        logger.info(f"Refreshing token with refresh token: {refresh_token}")
        result = await query_json("https://accounts.google.com/o/oauth2/token", q)
        if 'access_token' not in result:
            logger.error(f"Failed to refresh token: {result}")
            return None
        result['expires_at'] = (datetime.now() + timedelta(seconds=result['expires_in'])).timestamp()
        logger.info(f"Refreshed tokens: {result}")
        return result

@app.post('/refresh_token')
async def refresh_access_token(data: dict):
    if 'refresh_token' not in data:
        logger.error("Missing refresh token in request")
        raise HTTPException(status_code=400, detail="Missing refresh token")

    refresh_token = data['refresh_token']
    logger.info(f"Refreshing access token for refresh token: {refresh_token}")
    new_tokens = await AuthRefresh.refresh(refresh_token)

    if new_tokens:
        logger.info(f"New tokens obtained: {new_tokens}")
        return new_tokens
    else:
        logger.error("Failed to refresh access token")
        raise HTTPException(status_code=401, detail="Failed to refresh access token")

if __name__ == '__main__':
    logger.info("Starting server")
    uvicorn.run(app, host='0.0.0.0', port=8000)
