# spamaker
Another approach to help to C++ programmer to create SPA/PWA applications


## Usage

There is always one initial js file. For example main.js


```
spamake <type> src/main.js index.html
```

The command above expects, that you have source of the project in `src` folder.
It is recommended to expose this directory to local webserver to access these
files from the browser

```
--\
  |
  +--- src
  |     |
  |     +-- file.html
  |     +-- file.css
  |     +-- file.js
  |     +-- main.js
  |
  +--- index.html (generated)
  +--- index.js (generated)
  +--- index.css (generated)
```

## Build types


### • script

generates only Javascript files, doesn't generate html nor css. Useful to build
service/workers

### • html

generates only HTML file but no other files

### • packed

generates single page application combining html, js and css into single file. This can be useful for pages with no other references (such images, etc). Such page can be stored and downloaded as single file.

The word `packed` doesn't mean compression. However you can apply gzip compression on result and send the page compressed

### • page

generate all three files (html, css, js)

### • devel

generate HTML file while other files (js and css) are linked to the source folder. This allows to continue development in Chrome by directly modifying files

