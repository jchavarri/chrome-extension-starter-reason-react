open Chrome.Extensions;

open Chrome.Runtime;

[@bs.deriving jsConverter]
type color = [
  [@bs.as "White"] | `White
  [@bs.as "Pink"] | `Pink
  [@bs.as "Green"] | `Green
  [@bs.as "Yellow"] | `Yellow
];

/* Helper function from https://gist.github.com/Lokeh/a8d1dc6aa2043efa62b23e559291053e */
let thenResolve = (fn) => Js.Promise.then_((value) => Js.Promise.resolve(fn(value)));

let getCurrentTabUrl = () => {
  /* Query filter to be passed to chrome.tabs.query - see
     https://developer.chrome.com/extensions/tabs#method-query */
  let queryInfo = Tabs.mkQueryInfo(~active=Js.true_, ~currentWindow=Js.true_, ());
  Js.Promise.make(
    (~resolve, ~reject as _) =>
      Tabs.query(
        queryInfo,
        (tabs) => {
          /* chrome.tabs.query invokes the callback with a list of tabs that match the
             query. When the popup is opened, there is certainly a window and at least
             one tab, so we can safely assume that |tabs| is a non-empty array.
             A window can only have one active tab at a time, so the array consists of
             exactly one tab. */
          let tab = tabs[0];
          /* A tab is a plain object that provides information about the tab.
             See https://developer.chrome.com/extensions/tabs#type-Tab */
          let maybeUrl = Js.Nullable.to_opt(tab##url);
          /* tab.url is only available if the "activeTab" permission is declared.
             If you want to see the URL of other tabs (e.g. after removing active:true
             from |queryInfo|), then the "tabs" permission is required to see their
             "url" properties. */
          /* Js.assert_(Js.typeof(url) == "string", "tab.url should be a string"); */
          [@bs] resolve(maybeUrl)
        }
      )
  )
  /* Most methods of the Chrome extension APIs are asynchronous. This means that
     you CANNOT do something like this:

     var url;
     chrome.tabs.query(queryInfo, (tabs) => {
       url = tabs[0].url;
     });
     alert(url); // Shows "undefined", because chrome.tabs.query is async. */
};

/**
 * Change the background color of the current page.
 *
 * @param {string} color The new background color.
 */
let changeBackgroundColor = (maybeColor) =>
  /* See https://developer.chrome.com/extensions/tabs#method-executeScript.
     chrome.tabs.executeScript allows us to programmatically inject JavaScript
     into a page. Since we omit the optional first argument "tabId", the script
     is inserted into the active tab of the current window, which serves as the
     default. */
  Js.Promise.make(
    (~resolve, ~reject as _) =>
      Js.Option.(
        map(
          [@bs]
          (
            (color) => {
              let script = "document.body.style.backgroundColor=\"" ++ color ++ "\";";
              let scriptDetails = Tabs.mkScriptDetails(~code=script, ());
              Tabs.executeScriptWithCallback(scriptDetails, (_result) => [@bs] resolve(color))
            }
          ),
          maybeColor
        )
      ) |> ignore
  );

/**
 * Gets the saved background color for url.
 *
 * @param {string} url URL whose background color is to be retrieved.
 * @param {function(string)} callback called with the saved background color for
 *     the given url on success, or a falsy value if no color is retrieved.
 */
let getSavedBackgroundColor = (maybeUrl) => {
  let unwrapStoreValue = (maybeValue) =>
    switch maybeValue {
    | Some(Js.Json.JSONString(v)) => Some(v)
    | None => None
    | _ => failwith("Expected background color to be a string")
    };
  Js.Promise.make(
    (~resolve, ~reject as _) =>
      Js.Nullable.(
        iter(
          from_opt(maybeUrl),
          [@bs]
          (
            (url) =>
              [@bs.raw "console.log(chrome.storage.sync.get())"]
              Storage.Sync.get(
                url,
                (items) => {
                  /* See https://developer.chrome.com/apps/storage#type-StorageArea. We check
                     for chrome.runtime.lastError to ensure correctness even when the API call
                     fails. */
                  let restored =
                    test(lastError) ? unwrapStoreValue(Js.Dict.get(items, url)) : None;
                  [@bs] resolve(restored)
                }
              )
          )
        )
      )
  )
};

/**
 * Sets the given background color for url.
 *
 * @param {string} url URL for which background color is to be saved.
 * @param {string} color The background color to be saved.
 */
let saveBackgroundColor = (maybeUrl, color) =>
  /* See https://developer.chrome.com/apps/storage#type-StorageArea. We don't perform
     any work in the callback since we don't need to perform any action once the
      background color is saved. */
  switch maybeUrl {
  | Some(url) =>
    let items = Js.Dict.empty();
    Js.Dict.set(items, url, Js.Json.string(color));
    Storage.Sync.set(items, () => ())
  | None => failwith("Expected url to exist when saving background color")
  };

type action =
  | DidSelectColor(string)
  | DidRestoreColor(string)
  | DidGetUrl(option(string));

type state = {
  url: option(string),
  selectedColor: color
};

let component = ReasonReact.reducerComponent("ColorSelector");

/* This extension loads the saved background color for the current tab if one
   exists. The user can select a new background color from the dropdown for the
   current page, and it will be saved as part of the extension's isolated
   storage. The chrome.storage API is used for this purpose. This is different
   from the window.localStorage API, which is synchronous and stores data bound
   to a document's origin. Also, using chrome.storage.sync instead of
   chrome.storage.local allows the extension data to be synced across multiple
   user devices. */
let make = (_children) => {
  ...component,
  initialState: () => {url: None, selectedColor: `White},
  reducer: (action, state) =>
    switch action {
    | DidSelectColor(color) =>
      ReasonReact.UpdateWithSideEffects(
        {...state, selectedColor: Js.Option.getWithDefault(`White, colorFromJs(color))},
        (
          (self) => {
            changeBackgroundColor(Some(color)) |> ignore;
            saveBackgroundColor(self.state.url, color)
          }
        )
      )
    | DidRestoreColor(color) =>
      ReasonReact.UpdateWithSideEffects(
        {...state, selectedColor: Js.Option.getWithDefault(`White, colorFromJs(color))},
        ((_self) => changeBackgroundColor(Some(color)) |> ignore)
      )
    | DidGetUrl(maybeUrl) =>
      ReasonReact.UpdateWithSideEffects(
        {...state, url: maybeUrl},
        (
          (self) =>
            getSavedBackgroundColor(maybeUrl)
            |> thenResolve(
                 Js.Option.map([@bs] ((color) => self.reduce(() => DidRestoreColor(color), ())))
               )
            |> ignore
        )
      )
    },
  didMount: (_self) =>
    ReasonReact.SideEffects(
      (self) =>
        getCurrentTabUrl()
        |> thenResolve((maybeUrl) => self.reduce(() => DidGetUrl(maybeUrl), ()))
        |> ignore
    ),
  render: (self) =>
    <div>
      <span> (ReasonReact.stringToElement("Choose a color")) </span>
      <select
        id="dropdown"
        value=(colorToJs(self.state.selectedColor))
        onChange=(
          self.reduce(
            (event) =>
              DidSelectColor(ReactDOMRe.domElementToObj(ReactEventRe.Form.target(event))##value)
          )
        )>
        <option value=(colorToJs(`White))> (ReasonReact.stringToElement("White")) </option>
        <option value=(colorToJs(`Pink))> (ReasonReact.stringToElement("Pink")) </option>
        <option value=(colorToJs(`Green))> (ReasonReact.stringToElement("Green")) </option>
        <option value=(colorToJs(`Yellow))> (ReasonReact.stringToElement("Yellow")) </option>
      </select>
    </div>
};