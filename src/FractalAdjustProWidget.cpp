// Fractal scene picker and hardware-effect editor. AI-generated.
// SPDX-License-Identifier: GPL-2.0-or-later
#include "FractalAdjustProPlugin.h"
#include "ProfileManager.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

QWidget* FractalAdjustProPlugin::GetWidget()
{
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* panel = new QWidget;
    scroll->setWidget(panel);
    auto* layout = new QVBoxLayout(panel);
    auto* intro = new QLabel("Choose accessories, then apply a theme or custom lighting. Effects run on the hub after OpenRGB closes.");
    intro->setWordWrap(true);
    layout->addWidget(intro);
    auto* stream_note = new QLabel("For software animations, select Direct on the ‘Fractal Adjust Pro Direct (all accessories)’ device. "
                                  "Its LEDs are shared across the hub. Stop the animation source before choosing a Fractal theme.");
    stream_note->setWordWrap(true);
    layout->addWidget(stream_note);
    auto* resume = new QPushButton("Stop Direct — resume hardware effects");
    resume->setObjectName("stopDirect");
    layout->addWidget(resume);
    connect(resume, &QPushButton::clicked, panel, [this]()
    {
        for(auto& stream : streams) { stream->active_mode = 0; stream->UpdateMode(); }
    });
    auto* targets = new QListWidget;
    targets->setObjectName("fractalTargets");
    targets->setMaximumHeight(150);
    for(const auto& device : accessories)
    {
        auto* item = new QListWidgetItem(QString::fromStdString(device->name), targets);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
    auto* all = new QCheckBox("Select all accessories");
    all->setChecked(true);
    connect(all, &QCheckBox::toggled, panel, [targets](bool checked)
    {
        for(int i = 0; i < targets->count(); i++) targets->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    });
    layout->addWidget(all);
    layout->addWidget(targets);
    auto* status = new QLabel;
    status->setObjectName("fractalStatus");
    status->setWordWrap(true);
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);
    auto* theme_page = new QWidget;
    auto* grid = new QGridLayout(theme_page);
    tabs->addTab(theme_page, "Fractal themes");
    auto* brightness = new QSpinBox;
    brightness->setRange(0, 100);
    brightness->setValue(25);
    brightness->setSuffix("%");
    brightness->setObjectName("themeBrightness");
    grid->addWidget(new QLabel("Brightness"), 0, 0);
    grid->addWidget(brightness, 0, 1);
    auto apply = [this, targets, status](unsigned int selected_mode, unsigned int brightness, unsigned int speed,
                                       const std::vector<RGBColor>& colors, FractalThemes::Wave wave)
    {
        unsigned int done = 0, failed = 0;
        bool selected = false;
        for(int i = 0; i < targets->count(); i++) selected |= targets->item(i)->checkState() == Qt::Checked;
        if(selected) for(auto& stream : streams) { stream->active_mode = 0; stream->UpdateMode(); }
        for(unsigned int i = 0; i < accessories.size(); i++)
        {
            if(targets->item(i)->checkState() != Qt::Checked) continue;
            auto& device = *accessories[i];
            const int previous = device.active_mode;
            const mode backup = device.modes[selected_mode];
            mode& m = device.modes[selected_mode];
            m.brightness = brightness;
            m.speed = speed;
            if(m.flags & MODE_FLAG_HAS_MODE_SPECIFIC_COLOR) m.colors = colors;
            if(selected_mode == FractalThemes::Waves) m.direction = wave.Pack();
            device.active_mode = selected_mode;
            device.UpdateMode();
            if(device.last_update_ok) done++;
            else { failed++; device.active_mode = previous; m = backup; }
        }
        status->setText(failed ? QString("Applied to %1 accessories; %2 failed. Check the hub connection and close the Fractal browser tab.").arg(done).arg(failed) :
                       done ? QString("Applied and saved on %1 accessories.").arg(done) : "Select at least one accessory.");
    };
    for(unsigned int i = 0; i < FractalThemes::Count; i++)
    {
        const auto& theme = FractalThemes::themes[i];
        auto* button = new QPushButton(theme.name);
        button->setObjectName(QString("theme%1").arg(i));
        button->setMinimumHeight(64);
        QString gradient;
        for(unsigned int c = 0; c < theme.color_count; c++)
        {
            QColor color(theme.colors[3 * c], theme.colors[3 * c + 1], theme.colors[3 * c + 2]);
            gradient += QString("stop:%1 %2, ").arg(c / double(theme.color_count - 1)).arg(color.darker(240).name());
        }
        gradient.chop(2);
        button->setStyleSheet(QString("QPushButton { color: white; font-weight: bold; padding: 12px; border: 2px solid #555; border-radius: 6px; background: qlineargradient(x1:0,y1:0,x2:1,y2:1,%1); } QPushButton:hover, QPushButton:focus { border-color: white; }").arg(gradient));
        grid->addWidget(button, 1 + i / 3, i % 3);
        connect(button, &QPushButton::clicked, panel, [apply, brightness, i]()
        { apply(FractalThemes::FirstMode + i, brightness->value(), FractalThemes::themes[i].speed, {}, {}); });
    }
    auto* off = new QPushButton("RGB Off");
    off->setObjectName("fractalOff");
    grid->addWidget(off, 4, 1, 1, 2);
    connect(off, &QPushButton::clicked, panel, [apply]() { apply(FRACTAL_OFF, 100, 100, {}, {}); });

    auto* custom_page = new QWidget;
    auto* form = new QFormLayout(custom_page);
    tabs->addTab(custom_page, "Custom lighting");
    auto* family = new QComboBox;
    family->setObjectName("effectFamily");
    const unsigned int modes[] = {FractalThemes::Shift, FRACTAL_STATIC, FRACTAL_BREATHING,
                                  FractalThemes::Waves, FractalThemes::TwoColorFade, FractalThemes::LavaLamp};
    const char* names[] = {"Shift", "Still", "Breathe", "Waves", "Two color fade", "Lava lamp"};
    for(unsigned int i = 0; i < 6; i++) family->addItem(names[i], modes[i]);
    form->addRow("Lighting pattern", family);
    auto* preset = new QComboBox;
    preset->setObjectName("customPreset");
    form->addRow("Starting preset", preset);
    auto* palette_row = new QWidget;
    auto* palette_layout = new QHBoxLayout(palette_row);
    palette_layout->setContentsMargins(0, 0, 0, 0);
    std::vector<QLineEdit*> colors;
    std::vector<QPushButton*> swatches;
    for(unsigned int i = 0; i < 6; i++)
    {
        auto* column = new QVBoxLayout;
        auto* text = new QLineEdit;
        text->setObjectName(QString("palette%1").arg(i));
        text->setAccessibleName(QString("Color %1 hex RGB").arg(i + 1));
        text->setMaxLength(6);
        text->setMaximumWidth(90);
        auto* swatch = new QPushButton;
        swatch->setAccessibleName(QString("Choose color %1").arg(i + 1));
        swatch->setMinimumHeight(28);
        column->addWidget(swatch);
        column->addWidget(text);
        palette_layout->addLayout(column);
        colors.push_back(text);
        swatches.push_back(swatch);
        connect(text, &QLineEdit::textChanged, panel, [swatch](const QString& hex)
        {
            QColor color("#" + hex);
            if(color.isValid()) swatch->setStyleSheet("background-color: " + color.name() + "; border: 1px solid #888;");
        });
        connect(swatch, &QPushButton::clicked, panel, [text, panel]()
        {
            QColor color = QColorDialog::getColor(QColor("#" + text->text()), panel, "Choose lighting color");
            if(color.isValid()) text->setText(color.name().mid(1).toUpper());
        });
    }
    form->addRow("Colors (RGB hex)", palette_row);
    auto spin = [form](const char* label, const char* name, int max, int value)
    {
        auto* input = new QSpinBox;
        input->setObjectName(name);
        input->setRange(0, max);
        input->setValue(value);
        form->addRow(label, input);
        return input;
    };
    auto* speed = spin("Speed (%)", "customSpeed", 100, 40);
    auto* custom_brightness = spin("Brightness (%)", "customBrightness", 100, 25);
    auto* wave_box = new QGroupBox("Wave shape");
    auto* wave_form = new QFormLayout(wave_box);
    std::vector<QSpinBox*> wave_inputs;
    const char* wave_names[] = {"Ramp-up", "Wave LEDs", "Ramp-down", "Frequency"};
    const int wave_defaults[] = {6, 3, 6, 100};
    for(unsigned int i = 0; i < 4; i++)
    {
        auto* input = new QSpinBox;
        input->setObjectName(QString("wave%1").arg(i));
        input->setRange(0, i == 3 ? 100 : 25);
        input->setValue(wave_defaults[i]);
        wave_inputs.push_back(input);
        wave_form->addRow(wave_names[i], input);
    }
    form->addRow(wave_box);
    auto load_preset = [preset, family, colors, swatches, speed, wave_box, wave_inputs]()
    {
        if(preset->currentIndex() < 0) return;
        const auto& p = FractalThemes::Preset(preset->currentData().toUInt());
        speed->setEnabled(p.kind != 1);
        speed->setValue(p.speed);
        wave_box->setVisible(p.kind == 7);
        for(unsigned int i = 0; i < colors.size(); i++)
        {
            colors[i]->setVisible(i < p.color_count);
            swatches[i]->setVisible(i < p.color_count);
            colors[i]->setText(QColor(p.colors[3 * i], p.colors[3 * i + 1], p.colors[3 * i + 2]).name().mid(1).toUpper());
        }
        const int values[] = {p.up, p.width, p.down, p.frequency};
        for(unsigned int i = 0; i < 4; i++) wave_inputs[i]->setValue(values[i]);
    };
    auto reset = [family, preset, load_preset]()
    {
        unsigned int m = family->currentData().toUInt();
        unsigned int kind = m == FRACTAL_STATIC ? 1 : m == FRACTAL_BREATHING ? 2 : m == FractalThemes::Shift ? 0 : m == FractalThemes::Waves ? 7 : m == FractalThemes::TwoColorFade ? 8 : 6;
        preset->blockSignals(true);
        preset->clear();
        for(unsigned int i = 0; i < FractalThemes::PresetCount; i++)
            if(FractalThemes::Preset(i).kind == kind) preset->addItem(FractalThemes::Preset(i).name, i);
        preset->blockSignals(false);
        load_preset();
    };
    connect(preset, QOverload<int>::of(&QComboBox::currentIndexChanged), panel, [load_preset](int) { load_preset(); });
    connect(family, QOverload<int>::of(&QComboBox::currentIndexChanged), panel, [reset](int) { reset(); });
    reset();
    auto* buttons = new QHBoxLayout;
    auto* reset_button = new QPushButton("Reset pattern");
    auto* apply_button = new QPushButton("Apply custom lighting");
    apply_button->setObjectName("applyCustom");
    buttons->addWidget(reset_button);
    buttons->addWidget(apply_button);
    form->addRow(buttons);
    connect(reset_button, &QPushButton::clicked, panel, reset);
    connect(apply_button, &QPushButton::clicked, panel, [=]()
    {
        std::vector<RGBColor> palette;
        for(auto* input : colors)
        {
            if(input->isHidden()) continue;
            QColor color("#" + input->text());
            if(input->text().length() != 6 || !color.isValid()) { status->setText("Enter six hex digits for each color, for example 0074C5."); return; }
            palette.push_back(ToRGBColor(color.red(), color.green(), color.blue()));
        }
        FractalThemes::Wave wave{(unsigned int)wave_inputs[0]->value(), (unsigned int)wave_inputs[1]->value(),
                                (unsigned int)wave_inputs[2]->value(), (unsigned int)wave_inputs[3]->value()};
        apply(family->currentData().toUInt(), custom_brightness->value(), speed->value(), palette, wave);
    });
    auto read_current = [this, targets, family, colors, speed, custom_brightness, wave_inputs, status]()
    {
        const int row = targets->currentRow();
        if(row < 0 || row >= (int)accessories.size()) return;
        const auto& device = *accessories[row];
        unsigned int active = device.active_mode;
        const mode& m = device.modes[active];
        std::vector<RGBColor> palette = m.colors;
        FractalThemes::Wave wave;
        if(active >= FractalThemes::FirstMode && active < FractalThemes::Shift)
        {
            const auto& p = FractalThemes::themes[active - FractalThemes::FirstMode];
            active = p.kind == 0 ? FractalThemes::Shift : p.kind == 6 ? FractalThemes::LavaLamp : p.kind == 7 ? FractalThemes::Waves : FractalThemes::TwoColorFade;
            for(unsigned int i = 0; i < p.color_count; i++) palette.push_back(ToRGBColor(p.colors[3*i], p.colors[3*i+1], p.colors[3*i+2]));
        }
        else if(active == FractalThemes::Waves) wave = FractalThemes::Wave::Unpack(m.direction);
        const int index = family->findData(active);
        if(index < 0) { status->setText("Choose an accessory with a theme or custom lighting to edit."); return; }
        family->setCurrentIndex(index);
        speed->setValue(m.speed);
        custom_brightness->setValue(m.brightness);
        for(unsigned int i = 0; i < palette.size() && i < colors.size(); i++)
            colors[i]->setText(QColor(RGBGetRValue(palette[i]), RGBGetGValue(palette[i]), RGBGetBValue(palette[i])).name().mid(1).toUpper());
        const unsigned int values[] = {wave.up, wave.width, wave.down, wave.frequency};
        for(unsigned int i = 0; i < 4; i++) wave_inputs[i]->setValue(values[i]);
    };
    auto* edit_current = new QPushButton("Read highlighted accessory");
    connect(edit_current, &QPushButton::clicked, panel, read_current);
    form->addRow(edit_current);
    if(!accessories.empty()) targets->setCurrentRow(0);
    auto* profiles = new QComboBox;
    profiles->setObjectName("fractalProfiles");
    auto* save = new QPushButton("Save current scene…");
    auto* load = new QPushButton("Load scene");
    auto* profile_row = new QHBoxLayout;
    profile_row->addWidget(profiles, 1);
    profile_row->addWidget(load);
    profile_row->addWidget(save);
    layout->addLayout(profile_row);
    auto* manager = api->GetProfileManager();
    auto refresh = [manager, profiles]()
    {
        profiles->clear();
        for(const auto& name : manager->profile_list) profiles->addItem(QString::fromStdString(name));
    };
    refresh();
    connect(save, &QPushButton::clicked, panel, [=]()
    {
        bool ok;
        const QString name = QInputDialog::getText(panel, "Save scene", "New scene name (all devices):", QLineEdit::Normal, "", &ok).trimmed();
        if(!ok || name.isEmpty()) return;
        if(!QRegularExpression("^[\\p{L}\\p{N} _-]+$").match(name).hasMatch()) { status->setText("Use a scene name with letters, numbers, spaces, hyphens or underscores."); return; }
        const QString path = QString::fromStdString((api->GetConfigurationDirectory() / (name.toStdString() + ".orp")).string());
        if(QFileInfo::exists(path)) { status->setText("That scene already exists. Choose a new name."); return; }
        if(!manager->SaveProfile(name.toStdString()) || QFileInfo(path).size() <= 20) { status->setText("Could not save the scene."); return; }
        refresh();
        profiles->setCurrentText(name);
        status->setText("Scene saved. It includes the settings currently applied to all OpenRGB devices.");
    });
    connect(load, &QPushButton::clicked, panel, [=]()
    {
        if(profiles->currentText().isEmpty()) return;
        const bool loaded = manager->LoadProfile(profiles->currentText().toStdString());
        if(loaded) for(auto* device : api->GetRGBControllers()) device->UpdateMode();
        bool failed = false;
        if(loaded) read_current();
        for(const auto& device : accessories) failed |= !device->last_update_ok;
        status->setText(loaded && !failed ? "Scene loaded." : "Scene could not be fully applied. Check the device connection.");
    });
    auto* startup_page = new QWidget;
    auto* startup_form = new QFormLayout(startup_page);
    tabs->addTab(startup_page, "Startup effect");
    auto* startup_note = new QLabel("Lighting played when the hub powers on. Save here to change startup lighting; regular scenes remain separate.");
    startup_note->setWordWrap(true);
    startup_form->addRow(startup_note);
    auto* startup_mode = new QComboBox;
    startup_mode->setObjectName("startupMode");
    startup_mode->addItem("Meshify effect", 3);
    startup_mode->addItem("Fade in", 4);
    startup_mode->addItem("No effect (instant color)", 5);
    startup_mode->addItem("RGB Off", 10);
    startup_form->addRow("Power-on pattern", startup_mode);
    auto* startup_color = new QLineEdit("FFFFFF");
    startup_color->setObjectName("startupColor");
    startup_color->setMaxLength(6);
    startup_form->addRow("Color (RGB hex)", startup_color);
    auto* startup_brightness = new QSpinBox;
    startup_brightness->setObjectName("startupBrightness");
    startup_brightness->setRange(0, 100);
    startup_brightness->setValue(50);
    startup_brightness->setSuffix("%");
    startup_form->addRow("Brightness", startup_brightness);
    auto* startup_read = new QPushButton("Read highlighted accessory");
    auto* startup_save = new QPushButton("Save startup to selected accessories");
    startup_save->setObjectName("saveStartup");
    startup_form->addRow(startup_read);
    startup_form->addRow(startup_save);
    connect(startup_read, &QPushButton::clicked, panel, [=]()
    {
        int row = targets->currentRow();
        if(row < 0 || row >= (int)accessories.size()) return;
        FractalStartupEffect effect;
        if(!accessories[row]->hub->ReadStartup(accessories[row]->index, effect)) { status->setText("Could not read a supported startup effect."); return; }
        startup_mode->setCurrentIndex(startup_mode->findData(effect.kind));
        startup_color->setText(QColor(effect.color[0], effect.color[1], effect.color[2]).name().mid(1).toUpper());
        startup_brightness->setValue(effect.brightness);
        status->setText("Read startup settings from the highlighted accessory.");
    });
    connect(startup_save, &QPushButton::clicked, panel, [=]()
    {
        QColor color("#" + startup_color->text());
        if(startup_color->text().size() != 6 || !color.isValid()) { status->setText("Enter six hex digits for the startup color."); return; }
        FractalStartupEffect effect;
        effect.kind = startup_mode->currentData().toUInt();
        effect.brightness = startup_brightness->value();
        effect.color[0] = color.red(); effect.color[1] = color.green(); effect.color[2] = color.blue();
        unsigned int done = 0, failed = 0;
        for(unsigned int i = 0; i < accessories.size(); i++)
        {
            if(targets->item(i)->checkState() != Qt::Checked) continue;
            if(accessories[i]->hub->ApplyStartup(accessories[i]->index, effect)) done++; else failed++;
        }
        status->setText(failed ? QString("Saved startup on %1 accessories; %2 failed.").arg(done).arg(failed) :
                       done ? QString("Startup saved on %1 accessories. Verify its appearance at the next full power-on.").arg(done) : "Select at least one accessory.");
    });
    layout->addWidget(status);
    layout->addStretch();
    return scroll;
}
