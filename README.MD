[shinxbot2](https://github.com/Jayfeather233/shinxbot2) 的插件仓。

[Documents](./README_EN.md)/[文档](./README.md)

## 插件包含

- 恶臭数字论证器，来源：[itorr](https://github.com/itorr/homo)

- 来点二次元。来源：[dmoe](https://www.dmoe.cc)

- 首字母缩写识别，来源：[itorr](https://github.com/itorr/nbnhhsh)

- 在群或私聊中生成一个假的转发信息

- 来点色图：随机色卡

- 赛博养猫（与 gpt3.5 共同创作）

- cat image for http code。来源[httpcats](https://httpcats.com/)

- 图片OCR (qq自带的那个)

- 图片处理：对称、旋转、万花筒

- openai的 `gpt-3.5-turbo` API支持，且支持在消息中自动注入 `[User: ID]` 前缀以便 AI 识别群聊中的不同用户。

- google的 `gemini-pro[-vision]` API支持，同样支持在消息中自动注入 `[User: ID]` 前缀，[url](https://ai.google.dev/docs)

- StableDiffusion的 `SDXL-Turbo` API支持，[url](https://sdxlturbo.ai/)

- 群友聊天截图保存功能（美图）

- 龙王提醒

- 群成员变动提醒

- 人云亦云（复读）

- 不同群聊，私聊的消息互相转发

- 灰名单管理

- 管理多个bot，并（以后）可以通过广播传递消息

## 编译：

在 `functions|events` 目录下执行 `python generate_cmake.py`，然后 `./make_all.sh`。会将功能编译成`.so`放在`./lib/functions`和`./lib/events`下。将这两个文件夹复制到框架仓的`./lib/`下既可。

## 开发：

以 `class shinxbot` 为例，函数参数仅与消息相关，事件则接受 JSON 数据。当接收到聊天消息时应调用函数中的方法，处理其他提示消息时使用事件中的方法。

开发步骤如下：

1. 在 `./src/functions` 中创建一个新目录，将你的代码放入其中，编写一个继承自 `./src/interfaces/processable.h` 中类的类，实现其中的虚函数，并提供一个 `create` 方法以返回一个 `processable*` 类型的指针（可以参考 `functions/getJapaneseImage/Anime_Img.cpp`）。

2. 如果你继承类中的 `check()` 方法返回成功，则会执行 `process()` 方法。

3. 项目提供了一个 CMake 模板，在 `generate_cmake.py` 中，你可以使用它来编译你的功能模块，动态库将自动生成到 `./lib/functions/` 中。

4. 启动机器人。如果机器人已经在运行，你可以使用管理账户（在 `op_list.json` 中配置）发送命令 `bot.load [function|event] name` 来加载功能模块。

在运行机器人前，请确保你编写的功能动态库已放置在 `./lib/functions` 或 `./lib/events` 中，或使用 `generate_cmake.py` 和 `make_all.sh` 自动生成。

当机器人加载一个库时，会调用 `extern "C" processable *create()` 来获取函数指针。通常只需返回 `new YourFunc()` 即可。

当机器人卸载一个库时，会调用 `extern "C" processable *close(processable* p)` 来表明该库需要被卸载。请正确释放所有通过 `new` 获得的全局指针，包括参数 `p`。


TODO：

在框架仓里加入 `bot.add_plugin_repo <repo_name> <url>` 功能，表示将某个仓作加入插件来源；

`bot.compile_load_plugin <function/event> <repo_name/plugin_name>` 功能，表示编译并加载某个插件。

