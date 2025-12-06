# TactileBrowser TODO's

- [ ] Once external libs start working in Tactility, make sure it works properly and get #1 fixed
- [ ] Implement Gopher support
- [ ] Implement Gemini (the protocol) support  
      - [gmi100](https://github.com/ir33k/gmi100)  
      - [gemini](https://github.com/electrickite/gemini)  
- [ ] Improve CSS and HTML parsing
- [ ] Add image support, rendering is handled by device implementation (just provide a list of images to render and where)
- [ ] Add basic JS parsing
      - look into moddable (<https://moddable.com/>)
- [ ] better layout engine


## Done

- [x] Fix WASM implementation
  - [x] Resolve build errors (duplicate symbols)
  - [x] Expand CSS color parsing (hex, rgb/rgba, named colors)
  - [x] Implement text selection
  - [x] Enable URL bar input functionality
  - [x] Move inline style parsing to css_parser.cpp
  - [x] Set up proxy chain for HTML fetching
