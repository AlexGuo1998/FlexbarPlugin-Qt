#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QBuffer>
#include <QtGui/QPixmap>

#include "PluginTransport.h"

#define PLUGIN_COMMON_ARGS_DEF(timeout) \
    CallbackFunc callback = nullptr, void *context = nullptr, int timeoutMs = (timeout)
#define PLUGIN_COMMON_ARGS_CALL \
    callback, context, timeoutMs

class PluginOperation {
    using CallbackFunc = PluginTransport::CallbackFunc;

public:
    explicit PluginOperation(PluginTransport *transport) : transport(transport) {}

    /**
     * @brief Draw an image on a key.
     *
     * Detailed description:
     * This method sends a draw command to the server, allowing the key to be painted according to the format specified in the key object.
     *
     * @param serialNumber The serial number of the device.
     * @param key The key object received from the event `device.newPage` or `device.userData`.
     */
    void draw(const QString &serialNumber, const QJsonValue &key, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("draw"),
            QJsonObject{
                {QStringLiteral("serialNumber"), serialNumber},
                {QStringLiteral("type"), QLatin1String("draw")},
                {QStringLiteral("key"), key},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Draw an image on a key.
     *
     * Detailed description:
     * This method sends a draw command to the server, allowing the image data to be drawn on the specified key.
     *
     * @param serialNumber The serijal number of the device.
     * @param keyUid The uid attribute of the key object received from the event `plugin.alive`.
     * @param pngData The PNG image data as a `QByteArray`.
     */
    void draw(const QString &serialNumber, int keyUid, const QByteArray &pngData, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("draw"),
            QJsonObject{
                {QStringLiteral("serialNumber"), serialNumber},
                {QStringLiteral("type"), QLatin1String("base64")},
                {QStringLiteral("key"), QJsonObject{{QStringLiteral("uid"), keyUid}}},
                {
                    QStringLiteral("base64"),
                    QByteArrayLiteral("data:image/png;base64,") + QString::fromLatin1(pngData.toBase64())
                },
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Draw an image on a key.
     *
     * Detailed description:
     * This method sends a draw command to the server, allowing the image data to be drawn on the specified key.
     *
     * @param serialNumber The serijal number of the device.
     * @param keyUid The uid attribute of the key object received from the event `plugin.alive`.
     * @param pixmap The QPixmap containing the image.
     */
    void draw(const QString &serialNumber, int keyUid, const QPixmap &pixmap, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        pixmap.save(&buf, "PNG");
        buf.close();
        draw(serialNumber, keyUid, buf.data(), PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Send chart data for performance monitoring.
     *
     * Detailed description:
     * This method sends an array of performance metrics to be displayed in custom charts.
     * Each chart data object contains information about a specific metric including its
     * current value, unit, and display formatting.
     *
     * @param chartDataArray An array of chart data objects with the following structure:
     * @code{.json}
     * {
     *   "label": string,       // Display name of the metric
     *   "value": number|string, // Current value of the metric
     *   "unit": string,        // Unit of measurement (e.g., FPS, %, GB, ℃)
     *   "baseUnit": string,    // Base unit for conversion calculations
     *   "baseVal": number|string, // Raw value before formatting
     *   "maxLen": number,      // Maximum length for display formatting (1-4)
     *   "category": string,    // Category grouping (e.g., CPU, GPU, MEMORY, OTHER)
     *   "key": string,         // Unique identifier for the metric
     *   "icon": string?        // MDI icon name without 'mdi-' prefix (e.g., 'chevron-triple-right'). Search in https://pictogrammers.com/library/mdi/
     * }
     * @endcode
     */
    void sendChartData(const QJsonValue &chartDataArray, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("custom-chart-data"),
            QJsonObject{
                {QStringLiteral("data"), chartDataArray},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Update shortcuts.
     *
     * Detailed description:
     * This method sends a command to update the shortcuts.
     *
     * Shortcut values can be referenced from https://www.electronjs.org/docs/latest/api/accelerator
     *
     * @param shortcuts A JSON array of the shortcuts to update.
     * @code{.json}
     * [
     *   {
     *     "shortcut": "CommandOrControl+F2",     // The shortcut
     *     "action": "register"                   // "register" or "unregister"
     *   },
     *   {...}
     * ]
     * @endcode
     */
    void updateShortcuts(const QJsonValue &shortcuts, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("update-shortcuts"),
            QJsonObject{
                {QStringLiteral("shortcuts"), shortcuts},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Set data for a specific key.
     *
     * Detailed description:
     * This method sends a command to update the state or value of a specified key
     * based on the key type. It supports the "multiState" key type.
     *
     * @param serialNumber The serial number of the device.
     * @param keyUid The uid attribute of the key object received from the event `plugin.alive`.
     * @param state The key state index
     * @param message Optional message
     */
    void setMultiState(const QString &serialNumber, int keyUid, int state, const QString *message = nullptr, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        QJsonObject data{{QStringLiteral("state"), state}};
        if (message != nullptr) data[QLatin1String("message")] = *message;

        transport->call(
            QLatin1String("set"),
            QJsonObject{
                {QStringLiteral("serialNumber"), serialNumber},
                {QStringLiteral("key"), QJsonObject{{QStringLiteral("uid"), keyUid}}},
                {QStringLiteral("data"), data},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Set data for a specific key.
     *
     * Detailed description:
     * This method sends a command to update the state or value of a specified key
     * based on the key type. It supports the "slider" key type.
     *
     * @param serialNumber The serial number of the device.
     * @param keyUid The uid attribute of the key object received from the event `plugin.alive`.
     * @param value The slider value
     */
    void setSlider(const QString &serialNumber, int keyUid, double value, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("set"),
            QJsonObject{
                {QStringLiteral("serialNumber"), serialNumber},
                {QStringLiteral("key"), QJsonObject{{QStringLiteral("uid"), keyUid}}},
                {QStringLiteral("data"), QJsonObject{{QLatin1String("value"), value}}},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Show a snackbar message in the parent window.
     *
     * Detailed description:
     * Displays a transient message (snackbar) with a specified color and timeout.
     *
     * @param color The color type for the message. Possible values:
     *   - "success"
     *   - "info"
     *   - "warning"
     *   - "error"
     * @param message The message content to display
     * @param timeout Duration in milliseconds before hiding the message (default: 3000)
     */
    void showSnackbarMessage(const QString &color, const QString &message, int timeout = 3000, PLUGIN_COMMON_ARGS_DEF(5000)) const {
        transport->call(
            QLatin1String("ui-operation"),
            QJsonObject{
                {QStringLiteral("type"), QLatin1String("showSnackbarMessage")},
                {
                    QStringLiteral("data"), QJsonObject{
                        {QStringLiteral("color"), color},
                        {QStringLiteral("message"), message},
                        {QStringLiteral("timeout"), timeout}
                    }
                },
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Call Electron API, see https://www.electronjs.org/docs/latest/api/app for details.
     *
     * Detailed description:
     * This function allows you to call various Electron APIs through a single interface.
     *
     * @param api The Electron API method to call. Possible values:
     *   - "dialog.showOpenDialog"
     *   - "dialog.showSaveDialog"
     *   - "dialog.showMessageBox"
     *   - "dialog.showErrorBox"
     *   - "app.getAppPath"
     *   - "app.getPath"
     *   - "screen.getCursorScreenPoint"
     *   - "screen.getPrimaryDisplay"
     *   - "screen.getAllDisplays"
     *   - "screen.getDisplayNearestPoint"
     *   - "screen.getDisplayMatching"
     *   - "screen.screenToDipPoint"
     *   - "screen.dipToScreenPoint"
     * @param args Arguments to be passed to the Electron API call
     */
    void electronAPI(const QString &api, const QJsonArray &args, PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("callElectronAPI")},
                {
                    QStringLiteral("args"), QJsonObject{
                        {QStringLiteral("api"), api},
                        {QStringLiteral("args"), args}
                    }
                },
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Get application information.
     *
     * Detailed description:
     * This function retrieves the application version and platform.
     *
     * @return A promise that resolves with the app info in JSON format:
     * @code{.json}
     * {
     *   "version": "vX.X.X",
     *   "platform": "darwin | win32 | linux"
     * }
     * @endcode
     */
    void getAppInfo(PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("getAppInfo")},
                {QStringLiteral("args"), QJsonValue::Null},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Open a file from the given path.
     *
     * Detailed description:
     * Retrieves the file content if successful; otherwise throws an error.
     *
     * @param path The file path
     */
    void openFile(const QString &path, PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("openFile")},
                {
                    QStringLiteral("args"), QJsonObject{
                        {QStringLiteral("path"), path}
                    }
                },
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Save data to a specified file path.
     *
     * Detailed description:
     * Saves string or Buffer data to the provided file path and returns a status.
     *
     * @param path The file path
     * @param data The file content
     */
    void saveFile(const QString &path, const QString &data, PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("pluginSaveFile")},
                {
                    QStringLiteral("args"), QJsonObject{
                        {QStringLiteral("path"), path},
                        {QStringLiteral("data"), data},
                    }
                },
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Get the list of opened windows.
     *
     * Detailed description:
     * Retrieves an array of window objects in JSON format, each containing details
     * such as `platform`, `id`, `title`, `owner`, `bounds`, and `memoryUsage`.
     *
     * @return A promise that resolves to an array of window objects, for example:
     * @code{.json}
     * [
     *   {
     *     "platform": "windows",
     *     "id": 592082,
     *     "title": "Flexbar Designer",
     *     "owner": {
     *       "processId": 11860,
     *       "path": "Path to the executable",
     *       "name": "Flexbar Designer"
     *     },
     *     "bounds": {
     *       "x": 154,
     *       "y": 0,
     *       "width": 2252,
     *       "height": 1528
     *     },
     *     "memoryUsage": 188665856
     *   }
     * ]
     * @endcode
     */
    void getOpenedWindows(PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("getOpenedWindows")},
                {QStringLiteral("args"), QJsonValue::Null},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Get the device status.
     *
     * Detailed description:
     * Returns a JSON object containing various status fields such as
     * `connecting`, `connected`, `serialNumber`, `platform`, `profileVersion`,
     * and `fwVersion`.
     *
     * @return A promise that resolves to a JSON object, for example:
     * @code{.json}
     * {
     *   "connecting": true | false,
     *   "connected": true | false,
     *   "serialNumber": "XXXXXX",
     *   "platform": "win32 | darwin | linux",
     *   "profileVersion": "vX.X.X",
     *   "fwVersion": "vX.X.X"
     * }
     * @endcode
     */
    void getDeviceStatus(PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("getDeviceStatus")},
                {QStringLiteral("args"), QJsonValue::Null},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Get the plugin configuration.
     *
     * @param uuid The plugin UUID
     */
    void getConfig(const QString &uuid, PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("getPluginConfig")},
                {QStringLiteral("pluginID"), uuid},
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

    /**
     * @brief Set the plugin configuration.
     *
     * @param uuid The plugin UUID
     * @param config The plugin configuration object.
     */
    void setConfig(const QString &uuid, const QJsonValue &config, PLUGIN_COMMON_ARGS_DEF(0)) const {
        transport->call(
            QLatin1String("api-call"),
            QJsonObject{
                {QStringLiteral("api"), QLatin1String("setPluginConfig")},
                {QStringLiteral("pluginID"), uuid},
                {QStringLiteral("config"), config}
            },
            PLUGIN_COMMON_ARGS_CALL);
    }

private:
    PluginTransport *transport;
};

#undef PLUGIN_COMMON_ARGS_DEF
#undef PLUGIN_COMMON_ARGS_CALL
