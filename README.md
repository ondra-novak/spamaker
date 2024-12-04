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


### • develsl

generate HTML but create `sres` directory containing styles and scripts as symlinks


## directives

Directives are written to JS files as comments

```
//@directive
```
- `@require <script>` - link another script before current script. Circular referenes are allowed
- `@html <file>` - append HTML fragment to final HTML file if this file is included
- `@style <file>` - append CSS style to final style document if this file is included
- `@template <file>` - append template HTML file. It is included as <template id="<name>" >
- `@namespace <name>` - define namespace. It introduces namespace "<name>" to the script file. It also ensures that this script is wrapped into self contained
module (not for devel type)

