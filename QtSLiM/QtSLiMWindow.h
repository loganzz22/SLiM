#ifndef QTSLIMWINDOW_H
#define QTSLIMWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <QColor>

#include <string>
#include <vector>
#include <unordered_map>

#include "eidos_globals.h"
#include "slim_globals.h"
#include "eidos_rng.h"
#include "slim_sim.h"
//#include "slim_gui.h"
#include "QtSLiMExtras.h"
#include "QtSLiMPopulationTable.h"

class Subpopulation;
class QCloseEvent;
class QTextCursor;
class QtSLiMOutputHighlighter;
class QtSLiMScriptHighlighter;


namespace Ui {
class QtSLiMWindow;
}

class QtSLiMWindow : public QMainWindow
{
    Q_OBJECT    

private:
    // basic file i/o and change count management
    void init(void);
    bool maybeSave(void);
    void openFile(const QString &fileName);
    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QtSLiMWindow *findMainWindow(const QString &fileName) const;
    
    QString curFile;
    bool isUntitled = false, isRecipe = false;
    int slimChangeCount = 0;                    // private change count governing the recycle button's highlight
    
    // recent files
    enum { MaxRecentFiles = 10 };
    QAction *recentFileActs[MaxRecentFiles];
    
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    
    // state variables that are globals in Eidos and SLiM; we swap these in and out as needed, to provide each sim with its own context
    Eidos_RNG_State sim_RNG = {};
    slim_pedigreeid_t sim_next_pedigree_id = 0;
    slim_mutationid_t sim_next_mutation_id = 0;
    bool sim_suppress_warnings = false;
    std::string sim_working_dir;			// the current working dir that we will return to when executing SLiM/Eidos code
    std::string sim_requested_working_dir;	// the last working dir set by the user with the SLiMgui button/menu; we return to it on recycle

    // play-related variables; note that continuousPlayOn covers both profiling and non-profiling runs, whereas profilePlayOn
    // and nonProfilePlayOn cover those cases individually; this is for simplicity in enable bindings in the nib
    bool invalidSimulation_ = true, continuousPlayOn_ = false, profilePlayOn_ = false, nonProfilePlayOn_ = false;
    bool generationPlayOn_ = false, reachedSimulationEnd_ = false, hasImported_ = false;
    slim_generation_t targetGeneration_ = 0;
    QElapsedTimer continuousPlayElapsedTimer_;
    QTimer continuousPlayInvocationTimer_;
    uint64_t continuousPlayGenerationsCompleted_ = 0;
    QTimer generationPlayInvocationTimer_;
    int partialUpdateCount_ = 0;
    //SLiMPlaySliderToolTipWindow *playSpeedToolTipWindow;

#if (defined(SLIMGUI) && (SLIMPROFILING == 1))
    // profiling-related variables
    //NSDate *profileEndDate = nullptr;
    //clock_t profileElapsedCPUClock = 0;
    //eidos_profile_t profileElapsedWallClock = 0;
    //slim_generation_t profileStartGeneration = 0;
#endif
    
    QtSLiMPopulationTableModel *populationTableModel_ = nullptr;
    
    QtSLiMOutputHighlighter *outputHighlighter = nullptr;
    QtSLiMScriptHighlighter *scriptHighlighter = nullptr;
    
public:
    std::string scriptString;	// the script string that we are running on right now; not the same as the script textview!
    SLiMSim *sim = nullptr;		// the simulation instance for this window
    //SLiMgui *slimgui = nullptr;			// the SLiMgui Eidos class instance for this window

    // display-related variables
    //double fitnessColorScale, selectionColorScale;
    std::unordered_map<slim_objectid_t, QColor> genomicElementColorRegistry;
    bool zoomedChromosomeShowsRateMaps = false;
    bool zoomedChromosomeShowsGenomicElements = false;
    bool zoomedChromosomeShowsMutations = true;
    bool zoomedChromosomeShowsFixedSubstitutions = false;
    //bool reloadingSubpopTableview = false;

public:
    typedef enum {
        WF = 0,
        nonWF
    } ModelType;
    
    QtSLiMWindow(QtSLiMWindow::ModelType modelType);                        // untitled window
    explicit QtSLiMWindow(const QString &fileName);                         // window from a file
    QtSLiMWindow(const QString &recipeName, const QString &recipeScript);   // window from a recipe
    virtual ~QtSLiMWindow() override;
    
    void initializeUI(void);
    void tile(const QMainWindow *previous);
    void openRecipe(const QString &recipeName, const QString &recipeScript);   // called by QtSLiMAppDelegate to open a new recipe window
    
    static std::string defaultWFScriptString(void);
    static std::string defaultNonWFScriptString(void);

    static const QColor &blackContrastingColorForIndex(int index);
    void colorForGenomicElementType(GenomicElementType *elementType, slim_objectid_t elementTypeID, float *p_red, float *p_green, float *p_blue, float *p_alpha);
    
    std::vector<Subpopulation*> selectedSubpopulations(void);
    
    inline bool invalidSimulation(void) { return invalidSimulation_; }
    void setInvalidSimulation(bool p_invalid);
    inline bool reachedSimulationEnd(void) { return reachedSimulationEnd_; }
    void setReachedSimulationEnd(bool p_reachedEnd);
    void setContinuousPlayOn(bool p_flag);
    void setGenerationPlayOn(bool p_flag);
    void setNonProfilePlayOn(bool p_flag);
    
    void selectErrorRange(void);
    void checkForSimulationTermination(void);
    void startNewSimulationFromScript(void);
    void setScriptStringAndInitializeSimulation(std::string string);
    
    void updateOutputTextView(void);
    void updateGenerationCounter(void);
    void updateAfterTickFull(bool p_fullUpdate);
    void updatePlayButtonIcon(bool pressed);
    void updateProfileButtonIcon(bool pressed);
    void updateRecycleButtonIcon(bool pressed);
    void updateUIEnabling(void);

    void willExecuteScript(void);
    void didExecuteScript(void);
    bool runSimOneGeneration(void);
    void _continuousPlay(void);
    void playOrProfile(bool isPlayAction);
    void _generationPlay(void);
    
    void updateChangeCount(void);
    bool changedSinceRecycle(void);
    void resetSLiMChangeCount(void);
    void scriptTexteditChanged(void);
    
    bool checkScriptSuppressSuccessResponse(bool suppressSuccessResponse);    
    
signals:
    void terminationWithMessage(QString message);
    
public slots:
    void showTerminationMessage(QString terminationMessage);
    
    void playOneStepClicked(void);
    void playClicked(void);
    void profileClicked(void);
    void generationChanged(void);
    void recycleClicked(void);
    void playSpeedChanged(void);

    void showMutationsToggled(void);
    void showFixedSubstitutionsToggled(void);
    void showChromosomeMapsToggled(void);
    void showGenomicElementsToggled(void);

    void checkScriptClicked(void);
    void prettyprintClicked(void);
    void scriptHelpClicked(void);
    void showConsoleClicked(void);
    void showBrowserClicked(void);

    void clearOutputClicked(void);
    void dumpPopulationClicked(void);
    void graphPopupButtonClicked(void);
    void changeDirectoryClicked(void);
    
    void shiftSelectionLeft(void);
    void shiftSelectionRight(void);
    void commentUncommentSelection(void);

    //
    //  UI glue, defined in QtSLiMWindow_glue.cpp
    //
    
private slots:
    void displayFontPrefChanged();
    void scriptSyntaxHighlightPrefChanged();
    void outputSyntaxHighlightPrefChanged();
    
    void aboutQtSLiM();
    void showPreferences();
    
    void newFile_WF();
    void newFile_nonWF();
    void open();
    bool save();
    bool saveAs();
    void revert();
    void updateRecentFileActions();
    void openRecentFile();
    void clearRecentFiles();
    void documentWasModified();
    
    void playOneStepPressed(void);
    void playOneStepReleased(void);
    void playPressed(void);
    void playReleased(void);
    void profilePressed(void);
    void profileReleased(void);
    void recyclePressed(void);
    void recycleReleased(void);

    void showMutationsPressed(void);
    void showMutationsReleased(void);
    void showFixedSubstitutionsPressed(void);
    void showFixedSubstitutionsReleased(void);
    void showChromosomeMapsPressed(void);
    void showChromosomeMapsReleased(void);
    void showGenomicElementsPressed(void);
    void showGenomicElementsReleased(void);

    void checkScriptPressed(void);
    void checkScriptReleased(void);
    void prettyprintPressed(void);
    void prettyprintReleased(void);
    void scriptHelpPressed(void);
    void scriptHelpReleased(void);
    void showConsolePressed(void);
    void showConsoleReleased(void);
    void showBrowserPressed(void);
    void showBrowserReleased(void);

    void clearOutputPressed(void);
    void clearOutputReleased(void);
    void dumpPopulationPressed(void);
    void dumpPopulationReleased(void);
    void graphPopupButtonPressed(void);
    void graphPopupButtonReleased(void);
    void changeDirectoryPressed(void);
    void changeDirectoryReleased(void);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    QStringList linesForRoundedSelection(QTextCursor &cursor, bool &movedBack);
    
private:
    void glueUI(void);
    Ui::QtSLiMWindow *ui;
};

#endif // QTSLIMWINDOW_H





















