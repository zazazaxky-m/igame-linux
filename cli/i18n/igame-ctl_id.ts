<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="id_ID">
<context>
    <name>Cli</name>
    <message>
        <location filename="../main.cpp" line="27"/>
        <source>Usage: igame-ctl &lt;command&gt; [options]

  status                              GPU info, telemetry, lighting
  rgb &lt;effect&gt; [-c COLOR,...] [-s SPEED] [-b BRIGHTNESS] [--low N] [--high N] [--sens PERCENT]
                                      animated: static breathing rainbow_breathing spectrum cycle
                                      aurora plasma ocean heartbeat candle twinkle lightning glitch
                                      flash double_flash strobe police daytime off
                                      sensors: temperature load power
                                      audio (LedFx style): music_smart, music (Bar),
                                      music_spectrum (Energy), music_scroll (Scroll),
                                      music_bass (Power), music_strobe (BPM Strobe),
                                      music_realstrobe (Real Strobe), music_rainbow (Wavelength),
                                      music_fire (Magnitude), music_chill (Ambient)
                                      (audio effects need igame-control running, at least in the tray;
                                      --sens 10..300 = sound sensitivity, default 100)
  oc [--power W|default] [--core MHZ] [--mem MHZ] [--lock MIN:MAX|off]
  fan auto|PERCENT|curve TEMP:PERCENT,... [--fan N] [--zero-rpm TEMP|off]
                                      without --fan: all fans (kept in sync)
  profile list|save NAME|load NAME|delete NAME
  ledfx on|off [--network on|off]     let LedFx drive the light over DDP (port 4048)
  boot on|off                         apply the saved overclock when the service starts
  reset                               power limit and overclock back to defaults
  raw &apos;&lt;json&gt;&apos;                        send a raw request

Socket: %1
</source>
        <translation>Pemakaian: igame-ctl &lt;perintah&gt; [opsi]

  status                              info GPU, telemetri, lampu
  rgb &lt;efek&gt; [-c WARNA,...] [-s KEC] [-b TERANG] [--low N] [--high N] [--sens PERSEN]
                                      animasi: static breathing rainbow_breathing spectrum cycle
                                      aurora plasma ocean heartbeat candle twinkle lightning glitch
                                      flash double_flash strobe police daytime off
                                      sensor: temperature load power
                                      suara (gaya LedFx): music_smart, music (Bar),
                                      music_spectrum (Energy), music_scroll (Scroll),
                                      music_bass (Power), music_strobe (BPM Strobe),
                                      music_realstrobe (Real Strobe), music_rainbow (Wavelength),
                                      music_fire (Magnitude), music_chill (Ambient)
                                      (efek suara butuh igame-control jalan, minimal di tray;
                                      --sens 10..300 = sensitivitas suara, default 100)
  oc [--power W|default] [--core MHZ] [--mem MHZ] [--lock MIN:MAX|off]
  fan auto|PERSEN|curve SUHU:PERSEN,... [--fan N] [--zero-rpm SUHU|off]
                                      tanpa --fan: semua kipas (disamakan)
  profile list|save NAMA|load NAMA|delete NAMA
  ledfx on|off [--network on|off]     izinkan LedFx mengendalikan lampu lewat DDP (port 4048)
  boot on|off                         terapkan overclock tersimpan saat service mulai
  reset                               power limit dan overclock kembali ke default
  raw &apos;&lt;json&gt;&apos;                        kirim request mentah

Socket: %1
</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="61"/>
        <source>Cannot connect to igamed at %1: %2</source>
        <translation>Tidak bisa terhubung ke igamed di %1: %2</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="62"/>
        <source>Start it with: sudo systemctl enable --now igamed</source>
        <translation>Jalankan dengan: sudo systemctl enable --now igamed</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="82"/>
        <source>igamed did not answer</source>
        <translation>igamed tidak menjawab</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="96"/>
        <source>Error: %1</source>
        <translation>Error: %1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="129"/>
        <source>Temperature</source>
        <translation>Suhu</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="129"/>
        <source>%1 °C   load %2%</source>
        <translation>%1 °C   beban %2%</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="130"/>
        <source>Power</source>
        <translation>Daya</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="130"/>
        <source>%1 W / %2 W (range %3-%4)</source>
        <translation>%1 W / %2 W (rentang %3-%4)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="131"/>
        <source>Clocks</source>
        <translation>Clock</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="131"/>
        <source>core %1 MHz (offset %2), memory %3 MHz (offset %4)</source>
        <translation>core %1 MHz (offset %2), memori %3 MHz (offset %4)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="137"/>
        <source>Fan %1</source>
        <translation>Kipas %1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="137"/>
        <source>%1 RPM, %2% (%3)</source>
        <translation>%1 RPM, %2% (%3)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="138"/>
        <source>manual</source>
        <translation>manual</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="138"/>
        <source>driver</source>
        <translation>driver</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="144"/>
        <source>%1 model %2 type %3</source>
        <translation>%1 model %2 tipe %3</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="145"/>
        <source>not found - %1</source>
        <translation>tidak ditemukan - %1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="146"/>
        <source>Effect</source>
        <translation>Efek</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="150"/>
        <source>Profiles</source>
        <translation>Profil</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="153"/>
        <source>off</source>
        <translation>mati</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="156"/>
        <source>controlling the light, %1 fps from %2</source>
        <translation>mengendalikan lampu, %1 fps dari %2</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="157"/>
        <source>waiting</source>
        <translation>menunggu</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="159"/>
        <source>all networks</source>
        <translation>semua jaringan</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="162"/>
        <source>%1 (root)</source>
        <translation>%1 (root)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="163"/>
        <source>%1 (NOT root)</source>
        <translation>%1 (BUKAN root)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="179"/>
        <source>Name an effect, e.g. igame-ctl rgb static -c FF0000</source>
        <translation>Sebutkan efek, mis. igame-ctl rgb static -c FF0000</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="228"/>
        <source>Use --lock MIN:MAX, e.g. --lock 210:2600</source>
        <translation>Pakai --lock MIN:MAX, mis. --lock 210:2600</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="237"/>
        <source>Performance settings applied: %1</source>
        <translation>Pengaturan performa diterapkan: %1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="243"/>
        <source>Examples: igame-ctl fan auto | fan 60 | fan curve 40:30,60:50,80:100 [--fan 2]</source>
        <translation>Contoh: igame-ctl fan auto | fan 60 | fan curve 40:30,60:50,80:100 [--fan 2]</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="252"/>
        <source>No fan configuration yet (is the GPU ready?)</source>
        <translation>Konfigurasi kipas belum tersedia (GPU belum siap?)</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="258"/>
        <source>The fan number must be 1-%1</source>
        <translation>Nomor kipas harus 1-%1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="274"/>
        <source>A curve needs at least 2 TEMP:PERCENT points, e.g. 40:30,60:50,80:100</source>
        <translation>Kurva butuh minimal 2 titik SUHU:PERSEN, mis. 40:30,60:50,80:100</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="283"/>
        <source>Fan mode: auto, PERCENT or curve</source>
        <translation>Mode kipas: auto, PERSEN, atau curve</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="308"/>
        <source>All fans: %1</source>
        <translation>Semua kipas: %1</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="308"/>
        <source>Fan %1: %2</source>
        <translation>Kipas %1: %2</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="320"/>
        <source>No profiles yet.</source>
        <translation>Belum ada profil.</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="327"/>
        <source>Name the profile</source>
        <translation>Sebutkan nama profil</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="332"/>
        <source>Profile actions: list, save, load, delete</source>
        <translation>Aksi profil: list, save, load, delete</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="336"/>
        <source>Profile &quot;%1&quot;: %2</source>
        <translation>Profil &quot;%1&quot;: %2</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="374"/>
        <source>Performance back to defaults</source>
        <translation>Performa kembali ke default</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="379"/>
        <source>The overclock will be applied when the service starts</source>
        <translation>Overclock akan diterapkan saat service mulai</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="380"/>
        <source>The overclock will not be applied when the service starts</source>
        <translation>Overclock tidak diterapkan saat service mulai</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="389"/>
        <source>Usage: igame-ctl ledfx on|off [--network on|off]</source>
        <translation>Pemakaian: igame-ctl ledfx on|off [--network on|off]</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="393"/>
        <source>LedFx settings saved</source>
        <translation>Pengaturan LedFx disimpan</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="400"/>
        <source>Unknown command: %1</source>
        <translation>Perintah tidak dikenal: %1</translation>
    </message>
</context>
</TS>
