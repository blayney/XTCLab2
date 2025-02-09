#include <juce_gui_extra/juce_gui_extra.h>
#include <opencv2/opencv.hpp>

#include <fstream>
#include <sstream>
#include <functional>

class MainComponent;
static void showConfigPopup (MainComponent* parent);
static void showCVCalibrationDialog (MainComponent* parent);
static void showAcousticCalibrationDialog (MainComponent* parent);
static void showVolumeAdjustmentDialog (MainComponent* parent);

struct ConfigData
{
    juce::String fileName;
};

struct CVCalibrationData
{
    float speakerDistance = 0.0f;
    float leftAngle       = 0.0f;
    float rightAngle      = 0.0f;
    float cameraAngle     = 0.0f;
    float cameraToRight   = 0.0f;
    float cameraToLeft    = 0.0f;
};

struct AcousticCalibrationData
{
    int  inputDeviceIndex  = -1;
    int  outputDeviceIndex = -1;
    int  micLeftChannel    = -1;
    int  micRightChannel   = -1;
    int  spkLeftChannel    = -1;
    int  spkRightChannel   = -1;
    float calibratedVolume = 0.0f;
    float leftLatency      = 0.0f;
    float rightLatency     = 0.0f;
};

class ConfigPopupComponent : public juce::Component,
                             public juce::Button::Listener
{
public:
    ConfigPopupComponent(std::function<void (int, const juce::String&)> callback)
        : callbackFunction(callback)
    {
        fileEditor.setMultiLine(false);
        fileEditor.setText("");
        addAndMakeVisible(fileEditor);
        
        loadButton.setButtonText("Load");
        loadButton.addListener(this);
        addAndMakeVisible(loadButton);
        
        setupButton.setButtonText("Setup");
        setupButton.addListener(this);
        addAndMakeVisible(setupButton);
        
        cancelButton.setButtonText("Cancel");
        cancelButton.addListener(this);
        addAndMakeVisible(cancelButton);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        fileEditor.setBounds(area.removeFromTop(30));
        auto buttonArea = area.removeFromBottom(30);
        auto third = buttonArea.getWidth() / 3;
        loadButton.setBounds(buttonArea.removeFromLeft(third).reduced(5));
        setupButton.setBounds(buttonArea.removeFromLeft(third).reduced(5));
        cancelButton.setBounds(buttonArea.reduced(5));
    }
    
    void buttonClicked(juce::Button* b) override
    {
        if (b == &loadButton)
            callbackFunction(1, fileEditor.getText());
        else if (b == &setupButton)
            callbackFunction(2, fileEditor.getText());
        else if (b == &cancelButton)
            callbackFunction(0, fileEditor.getText());
    }
    
private:
    juce::TextEditor fileEditor;
    juce::TextButton loadButton, setupButton, cancelButton;
    std::function<void (int, const juce::String&)> callbackFunction;
};

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent()
    {
        topdownLabel.setText("Top-down view will display here.", juce::dontSendNotification);
        topdownLabel.setJustificationType(juce::Justification::centred);
        topdownLabel.setColour(juce::Label::backgroundColourId, juce::Colours::lightgrey);
        addAndMakeVisible(topdownLabel);

        addAndMakeVisible(cameraComponent);

        dataReadout.setText("Real-time data will display here.", juce::dontSendNotification);
        dataReadout.setJustificationType(juce::Justification::topLeft);
        dataReadout.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
        dataReadout.setColour(juce::Label::textColourId, juce::Colours::limegreen);
        dataReadout.setFont(juce::Font("Default", 15.0f, juce::Font::plain));
        addAndMakeVisible(dataReadout);

        setSize(1200, 800);

        juce::Timer::callAfterDelay(100, [this]
        {
            showConfigPopup(this);
        });

        startTimerHz(30);
    }

    ~MainComponent() override
    {
        if (cap.isOpened())
            cap.release();
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto leftPanel = area.removeFromLeft(area.getWidth() / 3);
        topdownLabel.setBounds(leftPanel);
        auto cameraArea = area.removeFromTop(area.getHeight() / 2);
        cameraComponent.setBounds(cameraArea);
        dataReadout.setBounds(area);
    }

    void processConfigResult(int result, const juce::String& text)
    {
        if (result == 1) // Load
        {
            if (text.isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                                       "No config", "No .xtc file specified.");
                return;
            }
            loadConfig(text);
        }
        else if (result == 2) // Setup
        {
            beginInitialisation();
        }
        else // Cancel
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    }

    void loadConfig(const juce::String& fileName)
    {
        juce::File f(fileName);
        if (!f.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                   "Error", "File not found.");
            return;
        }
        juce::var parsed;
        {
            juce::FileInputStream fin(f);
            if (fin.openedOk())
                parsed = juce::JSON::parse(fin.readEntireStreamAsString());
        }
        if (parsed.isVoid())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                   "Error", "Failed to parse JSON from config.");
            return;
        }
        configData.fileName = fileName;
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                               "Loaded", "Loaded config from " + fileName);
        goFilterCreationMode();
    }

    void beginInitialisation()
    {
        getTopLevelComponent()->setName("XTC Lab: Initialisation");
        selectCamera();
    }

    void selectCamera()
    {
        if (!cap.open(0))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                   "No Cameras Found",
                                                   "Failed to open default camera device.");
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                               "Camera", "Camera initialized successfully!");
    }

    void timerCallback() override
    {
        if (cap.isOpened())
        {
            cv::Mat frame;
            cap >> frame;
            if (!frame.empty())
            {
                cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
                juce::Image juceImg(juce::Image::RGB, frame.cols, frame.rows, false);
                {
                    juce::Image::BitmapData bd(juceImg, juce::Image::BitmapData::writeOnly);
                    for (int y = 0; y < frame.rows; ++y)
                    {
                        auto* dest = bd.getLinePointer(y);
                        auto* src  = frame.ptr(y);
                        memcpy(dest, src, frame.cols * 3);
                    }
                }
                juce::Image scaled = juceImg.rescaled(cameraComponent.getWidth(),
                                                       cameraComponent.getHeight(),
                                                       juce::Graphics::ResamplingQuality::mediumResamplingQuality);
                cameraComponent.setImage(scaled);
                cameraComponent.setImagePlacement(juce::RectanglePlacement::centred);
            }
        }
    }

    void goFilterCreationMode()
    {
        getTopLevelComponent()->setName("XTC Lab: Filter Creation Mode");
        topdownLabel.setText("Displaying top-down view with calibrated geometry.", juce::dontSendNotification);
    }

private:
    ConfigData configData;
    CVCalibrationData cvData;
    AcousticCalibrationData acousticData;

    juce::Label topdownLabel;
    juce::ImageComponent cameraComponent;
    juce::Label dataReadout;

    cv::VideoCapture cap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

// Helper function: Show the config popup using our custom component.
static void showConfigPopup(MainComponent* parent)
{
    auto configComp = new ConfigPopupComponent([parent] (int result, const juce::String& text)
    {
        parent->processConfigResult(result, text);
    });
    
    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle = "Load Config or Setup";
    opts.dialogBackgroundColour = juce::Colours::lightgrey;
    opts.content.setOwned(configComp);
    opts.content->setSize(400, 150);
    opts.launchAsync();
}

// Main application class
class XTCApplication : public juce::JUCEApplication
{
public:
    XTCApplication() {}

    const juce::String getApplicationName() override       { return "XTC Lab (JUCE + OpenCV)"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset(new MainWindow("XTC Lab", new MainComponent()));
        mainWindow->setUsingNativeTitleBar(true);
        mainWindow->centreWithSize(1200, 800);
        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void anotherInstanceStarted(const juce::String&) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, juce::Component* content)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, false);
            setContentOwned(content, true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(XTCApplication)