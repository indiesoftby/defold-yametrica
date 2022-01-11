# Yandex.Metrica for Defold

**⚠️ Work In Progress! Not Ready Yet ⚠️**

[Yandex.Metrica](https://metrika.yandex.com/) is a free of charge web analytics tool for websites.

## Supported Platforms

| Platform        | Status |
| --------------- | ------ |
| Browser (HTML5) | Supported ✅ |

## Installation & Usage

### 1. Create Yandex.Metrica Tag

You need a Yandex ID in order to work with Yandex.Metrica. If you don't have one, you first need to sign up.

On the page with the list of tags, [add a tag](https://metrica.yandex.com/add). The tag setup page opens. Then fill in the information:

- **Tag name.** The specified name is shown on the My tags page and in the upper menu for switching between tags. Use the name of your Defold game.
- **Site URL.** Put the address of your game on Yandex.Games like `yandex.ru/games/play/123456`.

Then accept the Terms of use, click Create a tag.

### 2. Add YaMetrica As Dependency

You can use it in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/).

Open your `game.project` file and in the dependencies field under project add the ZIP file of a [specific release](https://github.com/indiesoftby/defold-yametrica/tags).

Then add the following to your `game.project` file:

```
[yametrica]
counter_id = PUT_YOUR_COUNTER_ID_HERE
```

### 3. Call YaMetrica Methods

In your `main.script` call `not_bounce` to send information about a non-bounce. It means, for the Yandex.Metrica, that your game finished loading:

```lua
function init(self)
	if yametrica then
		yametrica.not_bounce()
	end
end
```

Then, for every in-game events like scene switching or level ending, call `hit`:

```lua
if yametrica then
    yametrica.hit("#event-name", { title: "Event Name" })
end
```

## Lua <-> JS

| Yandex.Metrica JS SDK | YaMetrica Lua API |
| --------------------- | ----------------- |
| `ym(XXXXXX, 'hit', url[, options])` | `yametrica.hit(url, [options])` |
| `ym(XXXXXX, 'notBounce', [options])` | `yametrica.not_bounce([options])` |
| `ym(XXXXXX, 'reachGoal', target[, params[, callback[, ctx]]])` | `yametrica.reach_goal(target, [params])` |

## Credits

Artsiom Trubchyk ([@aglitchman](https://github.com/aglitchman)) is the current YaMetrica owner within Indiesoft and is responsible for the open source repository.

This project uses the source code of [CJSON](https://github.com/mpx/lua-cjson).

### License

MIT license.
