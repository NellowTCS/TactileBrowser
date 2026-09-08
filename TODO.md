# TactileBrowser TODO's

- [ ] Once external libs start working in Tactility, make sure it works properly and get #1 fixed
- [ ] Implement Gopher support
- [ ] Implement Gemini (the protocol) support  
      - [gmi100](https://github.com/ir33k/gmi100)  
      - [gemini](https://github.com/electrickite/gemini)  
- [ ] Add image support, rendering is handled by device implementation (just provide a list of images to render and where)
- [ ] Add basic JS parsing
      - look into moddable (<https://moddable.com/>)
- [ ] Add Test262 conformance runner once a JS engine lands
      (engine-agnostic runner: frontmatter parser + test262 harness bundle +
      feature/skip manifest, wired through the fdm_core_tests build like the
      wpt reftest runner in tests/src/)
- [ ] better layout engine
