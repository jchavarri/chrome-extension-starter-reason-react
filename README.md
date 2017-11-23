# A Chrome Extension with ReasonReact
This is a starter example to build a Chrome extension using Reason and React. It is 100% based on the original started project ["Getting Started: Building a Chrome Extension"](https://developer.chrome.com/extensions/getstarted).

The project consists on placing a clickable icon right next to Chrome's Omnibox for easy access. Clicking that icon opens a popup window that allows the user to choose the background color of the current page. If the user had selected a background color for the page earlier, the extension will remember the user's choice and use it as the default, once the popup is clicked:

![Getting started preview](https://developer.chrome.com/static/images/gettingstarted-preview.png)

### Running

Run this project:

```
npm install
npm start
# in another tab
npm run webpack
```

After you see the webpack compilation succeed (the `npm run webpack` step), open up the nested html files in `src/*` (**no server needed!**). Then modify whichever file in `src` and refresh the page to see the changes.

**For more elaborate ReasonReact examples**, please see https://github.com/reasonml-community/reason-react-example

### TODO

Contributions welcomed :)

- [ ] Make production webpack config
- [ ] The project is based in this temporary branch of `bucklescript-chrome` (the bindings for Chrome API). `package.json` should be changed once / if the changes are merged upstream