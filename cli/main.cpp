// SPDX-License-Identifier: GPL-3.0-or-later
// igame-ctl: command line client for igamed.

#include "protocol.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QLocalSocket>
#include <QTextStream>
#include <QTranslator>

#include <cstdio>

namespace {

QTextStream out(stdout);
QTextStream err(stderr);

// Translation context for the messages below (English source, Indonesian in i18n/).
struct Cli {
    Q_DECLARE_TR_FUNCTIONS(Cli)
};

QString usage()
{
    return Cli::tr("Usage: igame-ctl <command> [options]\n"
                   "\n"
                   "  status                              GPU info, telemetry, lighting\n"
                   "  rgb <effect> [-c COLOR,...] [-s SPEED] [-b BRIGHTNESS] [--low N] [--high N] [--sens PERCENT]\n"
                   "                                      animated: static breathing rainbow_breathing spectrum cycle\n"
                   "                                      aurora plasma ocean heartbeat candle twinkle lightning glitch\n"
                   "                                      flash double_flash strobe police daytime off\n"
                   "                                      sensors: temperature load power\n"
                   "                                      audio (LedFx style): music_smart, music (Bar),\n"
                   "                                      music_spectrum (Energy), music_scroll (Scroll),\n"
                   "                                      music_bass (Power), music_strobe (BPM Strobe),\n"
                   "                                      music_realstrobe (Real Strobe), music_rainbow (Wavelength),\n"
                   "                                      music_fire (Magnitude), music_chill (Ambient)\n"
                   "                                      (audio effects need igame-control running, at least in the tray;\n"
                   "                                      --sens 10..300 = sound sensitivity, default 100)\n"
                   "  oc [--power W|default] [--core MHZ] [--mem MHZ] [--lock MIN:MAX|off]\n"
                   "  fan auto|PERCENT|curve TEMP:PERCENT,... [--fan N] [--zero-rpm TEMP|off]\n"
                   "                                      without --fan: all fans (kept in sync)\n"
                   "  profile list|save NAME|load NAME|delete NAME\n"
                   "  ledfx on|off [--network on|off]     let LedFx drive the light over DDP (port 4048)\n"
                   "  boot on|off                         apply the saved overclock when the service starts\n"
                   "  reset                               power limit and overclock back to defaults\n"
                   "  raw '<json>'                        send a raw request\n"
                   "\n"
                   "Socket: %1\n").arg(igame::socketPath());
}

class Client
{
public:
    bool connect()
    {
        m_socket.connectToServer(igame::socketPath());
        if (!m_socket.waitForConnected(2000)) {
            err << Cli::tr("Cannot connect to igamed at %1: %2").arg(igame::socketPath(), m_socket.errorString())
                << "\n" << Cli::tr("Start it with: sudo systemctl enable --now igamed") << "\n";
            return false;
        }
        return true;
    }

    QJsonObject call(QJsonObject request)
    {
        request[QStringLiteral("id")] = ++m_id;
        m_socket.write(igame::encode(request));
        m_socket.waitForBytesWritten(2000);
        while (true) {
            qsizetype newline;
            while ((newline = m_buffer.indexOf('\n')) >= 0) {
                const QJsonObject reply = QJsonDocument::fromJson(m_buffer.left(newline)).object();
                m_buffer.remove(0, newline + 1);
                if (reply.value(QStringLiteral("id")).toInt() == m_id)
                    return reply;
            }
            if (!m_socket.waitForReadyRead(5000))
                return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), Cli::tr("igamed did not answer")}};
            m_buffer += m_socket.readAll();
        }
    }

private:
    QLocalSocket m_socket;
    QByteArray m_buffer;
    int m_id = 0;
};

int finish(const QJsonObject &reply, const QString &okText = {})
{
    if (!reply.value(QStringLiteral("ok")).toBool()) {
        err << Cli::tr("Error: %1").arg(reply.value(QStringLiteral("error")).toString()) << "\n";
        return 1;
    }
    if (!okText.isEmpty())
        out << okText << "\n";
    return 0;
}

QString option(const QStringList &args, const QString &name, const QString &fallback = {})
{
    const int i = int(args.indexOf(name));
    return i >= 0 && i + 1 < args.size() ? args[i + 1] : fallback;
}

int status(Client &client)
{
    const QJsonObject r = client.call({{QStringLiteral("cmd"), QStringLiteral("hello")}});
    if (!r.value(QStringLiteral("ok")).toBool())
        return finish(r);
    const QJsonObject info = r.value(QStringLiteral("info")).toObject();
    const QJsonObject gpu = info.value(QStringLiteral("gpu")).toObject();
    const QJsonObject lim = info.value(QStringLiteral("limits")).toObject();
    const QJsonObject rgb = info.value(QStringLiteral("rgb")).toObject();
    const QJsonObject d = info.value(QStringLiteral("daemon")).toObject();
    const QJsonObject t = r.value(QStringLiteral("telemetry")).toObject();
    const QJsonObject cfg = r.value(QStringLiteral("config")).toObject();
    const auto n = [](const QJsonObject &o, const char *k) { return o.value(QLatin1String(k)).toVariant().toString(); };

    // Labels are padded to line up the values, whatever their length after translation.
    const auto line = [](const QString &label, const QString &value) {
        out << label.leftJustified(11) << ": " << value << "\n";
    };
    line(QStringLiteral("GPU"), QStringLiteral("%1 (%2, driver %3)").arg(n(gpu, "name"), n(gpu, "architecture"), n(gpu, "driver")));
    line(Cli::tr("Temperature"), Cli::tr("%1 °C   load %2%").arg(n(t, "temp"), n(t, "util")));
    line(Cli::tr("Power"), Cli::tr("%1 W / %2 W (range %3-%4)").arg(n(t, "power"), n(t, "power_limit"), n(lim, "power_min"), n(lim, "power_max")));
    line(Cli::tr("Clocks"), Cli::tr("core %1 MHz (offset %2), memory %3 MHz (offset %4)")
                                .arg(n(t, "clk_gfx"), n(t, "core_offset"), n(t, "clk_mem"), n(t, "mem_offset")));
    line(QStringLiteral("VRAM"), QStringLiteral("%1 / %2 MiB").arg(n(t, "vram_used"), n(t, "vram_total")));
    const QJsonArray fans = t.value(QStringLiteral("fans")).toArray();
    for (int i = 0; i < fans.size(); ++i) {
        const QJsonObject f = fans[i].toObject();
        line(Cli::tr("Fan %1").arg(i + 1), Cli::tr("%1 RPM, %2% (%3)").arg(n(f, "rpm"), n(f, "pct"),
                                                                          f.value(QStringLiteral("manual")).toBool() ? Cli::tr("manual") : Cli::tr("driver")));
    }
    if (!t.value(QStringLiteral("fan_error")).toString().isEmpty())
        out << "             ! " << t.value(QStringLiteral("fan_error_text")).toString() << "\n";
    const QJsonObject rgbCfg = cfg.value(QStringLiteral("rgb")).toObject();
    line(QStringLiteral("RGB"), rgb.value(QStringLiteral("present")).toBool()
                                    ? Cli::tr("%1 model %2 type %3").arg(n(rgb, "bus"), n(rgb, "model"), n(rgb, "type"))
                                    : Cli::tr("not found - %1").arg(n(rgb, "error_text")));
    line(Cli::tr("Effect"), QStringLiteral("%1 %2, LED %3")
                                .arg(n(rgbCfg, "effect"),
                                     QString::fromUtf8(QJsonDocument(rgbCfg.value(QStringLiteral("colors")).toArray()).toJson(QJsonDocument::Compact)),
                                     r.value(QStringLiteral("led")).toString()));
    line(Cli::tr("Profiles"), cfg.value(QStringLiteral("profiles")).toObject().keys().join(QStringLiteral(", ")));
    {
        const QJsonObject lf = r.value(QStringLiteral("ledfx")).toObject();
        const QString state = !lf.value(QStringLiteral("enabled")).toBool(true) ? Cli::tr("off")
                            : !lf.value(QStringLiteral("error")).toString().isEmpty() ? lf.value(QStringLiteral("error_text")).toString()
                            : lf.value(QStringLiteral("active")).toBool()
                                ? Cli::tr("controlling the light, %1 fps from %2").arg(lf.value(QStringLiteral("fps")).toInt()).arg(lf.value(QStringLiteral("source")).toString())
                                : Cli::tr("waiting");
        line(QStringLiteral("LedFx"), QStringLiteral("%1 (DDP %2:%3)").arg(state,
                                                                           lf.value(QStringLiteral("network")).toBool() ? Cli::tr("all networks") : QStringLiteral("127.0.0.1"))
                                          .arg(lf.value(QStringLiteral("port")).toInt(4048)));
    }
    line(QStringLiteral("igamed"), d.value(QStringLiteral("root")).toBool() ? Cli::tr("%1 (root)").arg(n(d, "version"))
                                                                           : Cli::tr("%1 (NOT root)").arg(n(d, "version")));
    return 0;
}

QJsonObject currentConfig(Client &client, bool *ok)
{
    const QJsonObject r = client.call({{QStringLiteral("cmd"), QStringLiteral("hello")}});
    *ok = r.value(QStringLiteral("ok")).toBool();
    if (!*ok)
        finish(r);
    return r.value(QStringLiteral("config")).toObject();
}

int rgb(Client &client, const QStringList &args)
{
    if (args.size() < 2) {
        err << Cli::tr("Name an effect, e.g. igame-ctl rgb static -c FF0000") << "\n";
        return 2;
    }
    bool ok = false;
    QJsonObject rgb = currentConfig(client, &ok).value(QStringLiteral("rgb")).toObject();
    if (!ok)
        return 1;
    rgb[QStringLiteral("effect")] = args[1];
    const QString colors = option(args, QStringLiteral("-c"));
    if (!colors.isEmpty()) {
        QJsonArray list;
        for (QString c : colors.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            c = c.trimmed().toUpper();
            list.append(c.startsWith(QLatin1Char('#')) ? c : QStringLiteral("#") + c);
        }
        rgb[QStringLiteral("colors")] = list;
    }
    if (const QString s = option(args, QStringLiteral("-s")); !s.isEmpty())
        rgb[QStringLiteral("speed")] = s.toDouble();
    if (const QString b = option(args, QStringLiteral("-b")); !b.isEmpty())
        rgb[QStringLiteral("brightness")] = b.toInt();
    if (const QString v = option(args, QStringLiteral("--low")); !v.isEmpty())
        rgb[QStringLiteral("low")] = v.toDouble();
    if (const QString v = option(args, QStringLiteral("--high")); !v.isEmpty())
        rgb[QStringLiteral("high")] = v.toDouble();
    if (const QString v = option(args, QStringLiteral("--sens")); !v.isEmpty())
        rgb[QStringLiteral("sensitivity")] = v.toInt();
    return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("set_rgb")}, {QStringLiteral("rgb"), rgb}}),
                  QStringLiteral("RGB: ") + args[1]);
}

int oc(Client &client, const QStringList &args)
{
    bool ok = false;
    QJsonObject oc = currentConfig(client, &ok).value(QStringLiteral("oc")).toObject();
    if (!ok)
        return 1;
    if (const QString p = option(args, QStringLiteral("--power")); !p.isEmpty())
        oc[QStringLiteral("power_w")] = p == QLatin1String("default") ? 0 : p.toDouble();
    if (const QString c = option(args, QStringLiteral("--core")); !c.isEmpty())
        oc[QStringLiteral("core_mhz")] = c.toInt();
    if (const QString m = option(args, QStringLiteral("--mem")); !m.isEmpty())
        oc[QStringLiteral("mem_mhz")] = m.toInt();
    if (const QString l = option(args, QStringLiteral("--lock")); !l.isEmpty()) {
        if (l == QLatin1String("off")) {
            oc[QStringLiteral("lock_enabled")] = false;
        } else {
            const QStringList parts = l.split(QLatin1Char(':'));
            if (parts.size() != 2) {
                err << Cli::tr("Use --lock MIN:MAX, e.g. --lock 210:2600") << "\n";
                return 2;
            }
            oc[QStringLiteral("lock_enabled")] = true;
            oc[QStringLiteral("lock_min")] = parts[0].toInt();
            oc[QStringLiteral("lock_max")] = parts[1].toInt();
        }
    }
    return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("set_oc")}, {QStringLiteral("oc"), oc}}),
                  Cli::tr("Performance settings applied: %1").arg(QString::fromUtf8(QJsonDocument(oc).toJson(QJsonDocument::Compact))));
}

int fan(Client &client, const QStringList &args)
{
    if (args.size() < 2) {
        err << Cli::tr("Examples: igame-ctl fan auto | fan 60 | fan curve 40:30,60:50,80:100 [--fan 2]") << "\n";
        return 2;
    }
    bool ok = false;
    QJsonObject fans = currentConfig(client, &ok).value(QStringLiteral("fans")).toObject();
    if (!ok)
        return 1;
    QJsonArray list = fans.value(QStringLiteral("fans")).toArray();
    if (list.isEmpty()) {
        err << Cli::tr("No fan configuration yet (is the GPU ready?)") << "\n";
        return 1;
    }
    const QString which = option(args, QStringLiteral("--fan"));
    const int target = which.isEmpty() ? -1 : which.toInt() - 1;
    if (target >= list.size() || (!which.isEmpty() && target < 0)) {
        err << Cli::tr("The fan number must be 1-%1").arg(list.size()) << "\n";
        return 2;
    }

    QJsonObject setting = list[target < 0 ? 0 : target].toObject();
    const QString mode = args[1];
    if (mode == QLatin1String("auto")) {
        setting[QStringLiteral("mode")] = QStringLiteral("auto");
    } else if (mode == QLatin1String("curve")) {
        QJsonArray curve;
        for (const QString &pair : option(args, QStringLiteral("curve")).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QStringList tp = pair.split(QLatin1Char(':'));
            if (tp.size() == 2)
                curve.append(QJsonArray{tp[0].toDouble(), tp[1].toDouble()});
        }
        if (curve.size() < 2) {
            err << Cli::tr("A curve needs at least 2 TEMP:PERCENT points, e.g. 40:30,60:50,80:100") << "\n";
            return 2;
        }
        setting[QStringLiteral("mode")] = QStringLiteral("curve");
        setting[QStringLiteral("curve")] = curve;
    } else {
        bool isNumber = false;
        const int pct = mode.toInt(&isNumber);
        if (!isNumber) {
            err << Cli::tr("Fan mode: auto, PERCENT or curve") << "\n";
            return 2;
        }
        setting[QStringLiteral("mode")] = QStringLiteral("fixed");
        setting[QStringLiteral("fixed")] = pct;
    }
    if (const QString z = option(args, QStringLiteral("--zero-rpm")); !z.isEmpty()) {
        setting[QStringLiteral("zero_rpm")] = z != QLatin1String("off");
        if (z != QLatin1String("off"))
            setting[QStringLiteral("zero_rpm_temp")] = z.toInt();
    }

    if (target < 0) {
        fans[QStringLiteral("sync")] = true;
        for (int i = 0; i < list.size(); ++i)
            list[i] = setting;
    } else {
        if (fans.value(QStringLiteral("sync")).toBool())
            for (int i = 1; i < list.size(); ++i)
                list[i] = list[0];
        fans[QStringLiteral("sync")] = false;
        list[target] = setting;
    }
    fans[QStringLiteral("fans")] = list;
    return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("set_fans")}, {QStringLiteral("fans"), fans}}),
                  target < 0 ? Cli::tr("All fans: %1").arg(mode) : Cli::tr("Fan %1: %2").arg(target + 1).arg(mode));
}

int profile(Client &client, const QStringList &args)
{
    const QString action = args.value(1);
    if (action == QLatin1String("list") || action.isEmpty()) {
        bool ok = false;
        const QJsonObject profiles = currentConfig(client, &ok).value(QStringLiteral("profiles")).toObject();
        if (!ok)
            return 1;
        if (profiles.isEmpty())
            out << Cli::tr("No profiles yet.") << "\n";
        for (auto it = profiles.begin(); it != profiles.end(); ++it)
            out << "- " << it.key() << "  (" << it.value().toObject().value(QStringLiteral("saved")).toString() << ")\n";
        return 0;
    }
    const QString name = args.value(2);
    if (name.isEmpty()) {
        err << Cli::tr("Name the profile") << "\n";
        return 2;
    }
    const QString cmd = QStringLiteral("profile_") + action;
    if (action != QLatin1String("save") && action != QLatin1String("load") && action != QLatin1String("delete")) {
        err << Cli::tr("Profile actions: list, save, load, delete") << "\n";
        return 2;
    }
    return finish(client.call({{QStringLiteral("cmd"), cmd}, {QStringLiteral("name"), name}}),
                  Cli::tr("Profile \"%1\": %2").arg(name, action));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTranslator translator; // Indonesian when the system language is Indonesian
    if (translator.load(QLocale(), QStringLiteral("igame-ctl"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QCoreApplication::installTranslator(&translator);

    const QStringList args = app.arguments().mid(1);
    if (args.isEmpty() || args[0] == QLatin1String("-h") || args[0] == QLatin1String("--help")) {
        out << usage();
        return args.isEmpty() ? 2 : 0;
    }
    if (args[0] == QLatin1String("--version")) {
        out << "igame-ctl " << igame::kVersion << "\n";
        return 0;
    }

    Client client;
    if (!client.connect())
        return 1;

    const QString cmd = args[0];
    if (cmd == QLatin1String("status"))
        return status(client);
    if (cmd == QLatin1String("rgb"))
        return rgb(client, args);
    if (cmd == QLatin1String("oc"))
        return oc(client, args);
    if (cmd == QLatin1String("fan"))
        return fan(client, args);
    if (cmd == QLatin1String("profile"))
        return profile(client, args);
    if (cmd == QLatin1String("reset"))
        return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("reset_oc")}}), Cli::tr("Performance back to defaults"));
    if (cmd == QLatin1String("boot")) {
        const bool on = args.value(1) == QLatin1String("on");
        return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("set_options")},
                                   {QStringLiteral("options"), QJsonObject{{QStringLiteral("apply_oc_on_boot"), on}}}}),
                      on ? Cli::tr("The overclock will be applied when the service starts")
                         : Cli::tr("The overclock will not be applied when the service starts"));
    }
    if (cmd == QLatin1String("ledfx")) {
        QJsonObject options;
        if (args.value(1) == QLatin1String("on") || args.value(1) == QLatin1String("off"))
            options[QStringLiteral("ledfx_enabled")] = args.value(1) == QLatin1String("on");
        if (const QString n = option(args, QStringLiteral("--network")); !n.isEmpty())
            options[QStringLiteral("ledfx_network")] = n == QLatin1String("on");
        if (options.isEmpty()) {
            err << Cli::tr("Usage: igame-ctl ledfx on|off [--network on|off]") << "\n";
            return 2;
        }
        return finish(client.call({{QStringLiteral("cmd"), QStringLiteral("set_options")}, {QStringLiteral("options"), options}}),
                      Cli::tr("LedFx settings saved"));
    }
    if (cmd == QLatin1String("raw")) {
        const QJsonObject request = QJsonDocument::fromJson(args.value(1).toUtf8()).object();
        out << QJsonDocument(client.call(request)).toJson(QJsonDocument::Indented);
        return 0;
    }
    err << Cli::tr("Unknown command: %1").arg(cmd) << "\n\n" << usage();
    return 2;
}
