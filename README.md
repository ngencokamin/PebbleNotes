# PebbleNotes Revived
## Google Tasks client for Pebble smart watches

Read-only access and an ability to mark tasks as complete or incomplete with sorting options and due dates. Edit tasks on your preferred app and watch them appear on your watch as long as you have internet access.

<p align="center"> 
	<a href="https://github.com/ngencokamin/PebbleNotes/actions/workflows/main.yaml?query=branch%3Amaster">
		<img alt="GitHub Actions Workflow Status" src="https://github.com/ngencokamin/PebbleNotes/actions/workflows/main.yaml/badge.svg?branch=master" />
	</a> 
	<a href="https://www.buymeacoffee.com/ngencokamin">
		<img alt="Buy Me A Coffee" src="https://gist.githubusercontent.com/juliomaqueda/1d4399f36b7350d6a73db6a470826076/raw/3c5d5e222f1805c2698227e4eb9c5458a8742b75/buy_me_a_coffee_badge.svg" />
	</a> 
</p>

## Notes

This is very much a work in progress. Currently login works, but something is off with the refresh token logic and the app does not stay logged in. The current plan is to implement Google oauth in the pebble settings, which will require *another* total rewrite of the authentication code. Hopefully this will still work on iOS, but I have no way to test. If anyone wants to help test, feel free to check out the master build above. You can use [this docker image](https://github.com/ngencokamin/pebble-docker) to easily sideload the app to your watch without dealing with dependency hell.

Additionally, **I am not a C dev**. Any changes I make are only gonna be for the settings app, the JavaScript connector, and the authentication code. If anyone comfortable working in C/with experience writing Pebble Watch apps wants to help, feel free to make a PR against this repo or reach out at ngencokamin@gmail.com.

## Requirements

~~Program must work on both Android and iOS devices, because it uses
JavaScript backend for accessing tasks.~~

Will work on Android, potentially on iOS.
Requires active internet connection.

## ToDo
#### ngencokamin

- [x] Rewrite authentication website in Python 3
- [x] Host new version of auth website
- [ ] Rewrite settings in Pebble app to use Google oauth instead of relying on secondary website
  - [ ] Update settings and Javascript connector to work with new authentication method
  - [ ] Figure out refresh token issue leading to timeouts

#### Potential C Dev(s)

- [x] 1st generation of app interface (tree-like) **Credit MarSoft**
- [ ] 2nd generation of app interface (saving positions, etc)
- [ ] Realtime information updates
- [ ] Cache lists/tasks for offline access

## Credit

