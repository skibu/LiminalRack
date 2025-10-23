#include <app/MidiDisplay.hpp>
#include <ui/MenuSeparator.hpp>
#include <helpers.hpp>


namespace rack {
namespace app {


struct MidiDriverValueItem : ui::MenuItem {
	midi::Port* port;
	int driverId;
	void onAction(const ActionEvent& e) override {
		port->setDriverId(driverId);
	}
};

static void appendMidiDriverMenu(ui::Menu* menu, midi::Port* port) {
	if (!port)
		return;

	for (int driverId : midi::getDriverIds()) {
		MidiDriverValueItem* item = new MidiDriverValueItem;
		item->port = port;
		item->driverId = driverId;
		item->setText(midi::getDriver(driverId)->getName());
		item->setRightText(CHECKMARK(item->driverId == port->getDriverId()));
		menu->addChild(item);
	}
}

void MidiDriverChoice::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->addChild(createMenuLabel(string::translate("MidiDisplay.driver")));
	appendMidiDriverMenu(menu, port);
}

void MidiDriverChoice::step() {
	text = (port && port->driver) ? port->getDriver()->getName() : "";
	if (text.empty()) {
		text = "(" + string::translate("MidiDisplay.noDriver") + ")";
		color.a = 0.5f;
	}
	else {
		color.a = 1.f;
	}
}

struct MidiDriverItem : ui::MenuItem {
	midi::Port* port;
	ui::Menu* createChildMenu() override {
		ui::Menu* menu = new ui::Menu;
		appendMidiDriverMenu(menu, port);
		return menu;
	}
};


struct MidiDeviceValueItem : ui::MenuItem {
	midi::Port* port;
	int deviceId;
	void onAction(const ActionEvent& e) override {
		port->setDeviceId(deviceId);
	}
};

static void appendMidiDeviceMenu(ui::Menu* menu, midi::Port* port) {
	if (!port)
		return;

	{
		MidiDeviceValueItem* item = new MidiDeviceValueItem;
		item->port = port;
		item->deviceId = -1;
		item->setText("(" + string::translate("MidiDisplay.noDevice") + ")");
		item->setRightText(CHECKMARK(item->deviceId == port->getDeviceId()));
		menu->addChild(item);
	}

	for (int deviceId : port->getDeviceIds()) {
		MidiDeviceValueItem* item = new MidiDeviceValueItem;
		item->port = port;
		item->deviceId = deviceId;
		item->setText(port->getDeviceName(deviceId));
		item->setRightText(CHECKMARK(item->deviceId == port->getDeviceId()));
		menu->addChild(item);
	}
}

void MidiDeviceChoice::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->addChild(createMenuLabel(string::translate("MidiDisplay.device")));
	appendMidiDeviceMenu(menu, port);
}

void MidiDeviceChoice::step() {
	text = (port && port->device) ? port->getDevice()->getName() : "";
	if (text.empty()) {
		text = "(" + string::translate("MidiDisplay.noDevice") + ")";
		color.a = 0.5f;
	}
	else {
		color.a = 1.f;
	}
}

struct MidiDeviceItem : ui::MenuItem {
	midi::Port* port;
	ui::Menu* createChildMenu() override {
		ui::Menu* menu = new ui::Menu;
		appendMidiDeviceMenu(menu, port);
		return menu;
	}
};


struct MidiChannelValueItem : ui::MenuItem {
	midi::Port* port;
	int channel;
	void onAction(const ActionEvent& e) override {
		port->setChannel(channel);
	}
};

static void appendMidiChannelMenu(ui::Menu* menu, midi::Port* port) {
	if (!port)
		return;

	for (int channel : port->getChannels()) {
		MidiChannelValueItem* item = new MidiChannelValueItem;
		item->port = port;
		item->channel = channel;
		item->setText(port->getChannelName(channel));
		item->setRightText(CHECKMARK(item->channel == port->getChannel()));
		menu->addChild(item);
	}
}

void MidiChannelChoice::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->addChild(createMenuLabel(string::translate("MidiDisplay.channel")));
	appendMidiChannelMenu(menu, port);
}

void MidiChannelChoice::step() {
	text = port ? port->getChannelName(port->getChannel()) : string::translate("MidiDisplay.channel1");
}

struct MidiChannelItem : ui::MenuItem {
	midi::Port* port;
	ui::Menu* createChildMenu() override {
		ui::Menu* menu = new ui::Menu;
		appendMidiChannelMenu(menu, port);
		return menu;
	}
};


void MidiDisplay::setMidiPort(midi::Port* port) {
	clearChildren();

	math::Vec pos;

	MidiDriverChoice* driverChoice = createWidget<MidiDriverChoice>(pos);
	driverChoice->setWidth(getWidth());
	driverChoice->port = port;
	addChild(driverChoice);
	pos = driverChoice->getBox().getBottomLeft();
	this->driverChoice = driverChoice;

	this->driverSeparator = createWidget<LedDisplaySeparator>(pos);
	this->driverSeparator->setWidth(getWidth());
	addChild(this->driverSeparator);

	MidiDeviceChoice* deviceChoice = createWidget<MidiDeviceChoice>(pos);
	deviceChoice->setWidth(getWidth());
	deviceChoice->port = port;
	addChild(deviceChoice);
	pos = deviceChoice->getBox().getBottomLeft();
	this->deviceChoice = deviceChoice;

	this->deviceSeparator = createWidget<LedDisplaySeparator>(pos);
	this->deviceSeparator->setWidth(getWidth());
	addChild(this->deviceSeparator);

	MidiChannelChoice* channelChoice = createWidget<MidiChannelChoice>(pos);
	channelChoice->setWidth(getWidth());
	channelChoice->port = port;
	addChild(channelChoice);
	this->channelChoice = channelChoice;
}


void MidiButton::setMidiPort(midi::Port* port) {
	this->port = port;
}


void MidiButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	appendMidiMenu(menu, port);
}


void appendMidiMenu(ui::Menu* menu, midi::Port* port) {
	menu->addChild(createMenuLabel(string::translate("MidiDisplay.driver")));
	appendMidiDriverMenu(menu, port);

	menu->addChild(new ui::MenuSeparator);
	menu->addChild(createMenuLabel(string::translate("MidiDisplay.device")));
	appendMidiDeviceMenu(menu, port);

	menu->addChild(new ui::MenuSeparator);
	// menu->addChild(createMenuLabel(string::translate("MidiDisplay.channel")));
	// appendMidiChannelMenu(menu, port);

	// Uncomment this to use sub-menus instead of one big menu.

	// MidiDriverItem* driverItem = createMenuItem<MidiDriverItem>(string::translate("MidiDisplay.driver"), RIGHT_ARROW);
	// driverItem->port = port;
	// menu->addChild(driverItem);

	// MidiDeviceItem* deviceItem = createMenuItem<MidiDeviceItem>(string::translate("MidiDisplay.device"), RIGHT_ARROW);
	// deviceItem->port = port;
	// menu->addChild(deviceItem);

	MidiChannelItem* channelItem = createMenuItem<MidiChannelItem>(string::translate("MidiDisplay.channel"), RIGHT_ARROW);
	channelItem->port = port;
	menu->addChild(channelItem);
}


} // namespace app
} // namespace rack
