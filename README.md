# taskfinder
An app for searching for SpotPass (BOSS) tasks. Developed by ZeroSkill.

## Usage

1. To compile & use, you must have the `libjannson-dev` and `libcurl4` packages. You must also have `build-essential`. Build it with `make`.
2. Once compiled, the syntax is as follows:
   ```
   taskfinder [ctr or wup]-boss-apps.json [ctr or wup] task_name_to_search_goes_here
   ```
   
## Notes
You must provide a ctr or wup boss apps json in the same directory as the executable for it to work.

Running this will search every app in the json for the specified task using asynchronous instances of curl. It will go up to 256 of these instances.

With a fast internet connection, a single 3DS task search will finish in under an hour.

If you need support, you know where to find me. Good luck out there, soldier.
