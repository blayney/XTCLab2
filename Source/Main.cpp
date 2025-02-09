#include <juce_gui_basics/juce_gui_basics.h>
#include <opencv2/opencv.hpp>

class MainComponent : public juce::Component
{
public:
    MainComponent()
    {
        setSize (400, 300);
        cv::Mat testImage = cv::Mat::zeros(100, 100, CV_8UC3);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::white);
        g.setColour (juce::Colours::black);
        g.drawText ("Hello, JUCE + OpenCV!", getLocalBounds(),
                    juce::Justification::centred, true);
    }
};

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow (const juce::String& name)
        : DocumentWindow (name,
                          juce::Desktop::getInstance().getDefaultLookAndFeel()
                              .findColour (ResizableWindow::backgroundColourId),
                          DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new MainComponent(), true);

        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class HelloJUCEApplication : public juce::JUCEApplication
{
public:
    HelloJUCEApplication() {}

    const juce::String getApplicationName() override       { return "HelloJUCE"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> ("HelloJUCE");
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void anotherInstanceStarted (const juce::String&) override {}

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (HelloJUCEApplication)