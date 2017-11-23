const path = require('path');
const CopyWebpackPlugin = require('copy-webpack-plugin');

module.exports = {
  entry: './lib/js/src/popup.bs.js',
  output: {
    path: path.join(__dirname, "build"),
    filename: 'index.js',
  },
  plugins: [
    new CopyWebpackPlugin([
      { from: 'manifest.json'},
      { from: 'icon.png' },
      { from: 'popup.html' }
    ])
  ]
};
