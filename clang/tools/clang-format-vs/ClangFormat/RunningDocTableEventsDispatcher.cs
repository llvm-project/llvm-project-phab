using EnvDTE;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System.Linq;
using System.Collections.Generic;
#if DEBUG
using System.Runtime.InteropServices;
#endif

namespace LLVM.ClangFormat
{
    // Exposes event sources for IVsRunningDocTableEvents3 events.
    internal sealed class RunningDocTableEventsDispatcher : IVsRunningDocTableEvents3
    {
        private RunningDocumentTable _runningDocumentTable;
        private DTE _dte;
        private SolutionEvents _solutionEvents;
        private bool _isOpened = false;
        private bool _isClosing = false;
        private List<uint> _docCookiesShownBeforeOpened = new List<uint>();

#if DEBUG
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern void OutputDebugString(string message);
        public void DebugLog(string s)
        {
            OutputDebugString("clang-format-vs (RDT): " + s + "\n");
        }
#else
        public void DebugLog(string s) {}
#endif

        public delegate void DocumentHander(object sender, Document document);
        public event DocumentHander BeforeSave;
        public event DocumentHander BeforeShow;

        private void SolutionIsClosing()
        {
            _isOpened = false;
            _isClosing = true;
            DebugLog("SolutionIsClosing");
        }

        private void SolutionOpened()
        {
            _isOpened = true;
            _isClosing = false;
            DebugLog("SolutionOpened");
            foreach (var docCookie in _docCookiesShownBeforeOpened)
            {
                DebugLog("delay-formatting " + docCookie);
                OnAfterSave(docCookie);
            }
            _docCookiesShownBeforeOpened.Clear();
        }

        private Document FindDocumentByCookie(uint docCookie)
        {
            var documentInfo = _runningDocumentTable.GetDocumentInfo(docCookie);
            return _dte.Documents.Cast<Document>().FirstOrDefault(doc => doc.FullName == documentInfo.Moniker);
        }

        private void SavingDocument(uint docCookie)
        {
            if (BeforeSave != null)
            {
                var document = FindDocumentByCookie(docCookie);
                if (document != null) // Not sure why this happens sometimes
                {
                    BeforeSave(this, document);
                }
            }
        }

        private void ShowingDocument(uint docCookie)
        {
            if (BeforeShow != null)
            {
                if (!_isOpened)
                {
                    DebugLog("delaying formatting for " + docCookie);
                    _docCookiesShownBeforeOpened.Add(docCookie);
                }
                else
                {
                    var document = FindDocumentByCookie(docCookie);
                    if (document != null) // Not sure why this happens sometimes
                    {
                        BeforeShow(this, document);
                    }
                }
            }
        }

        public RunningDocTableEventsDispatcher(Package package)
        {
            DebugLog("RunningDocTableEventsDispatcher ctor");
            _runningDocumentTable = new RunningDocumentTable(package);
            _runningDocumentTable.Advise(this);
            _dte = (DTE)Package.GetGlobalService(typeof(DTE));
            _solutionEvents = _dte.Events.SolutionEvents;
            _solutionEvents.Opened += SolutionOpened;
            _solutionEvents.BeforeClosing += SolutionIsClosing;
        }

        public void FormatOpenDocuments()
        {
            foreach (var docInfo in _runningDocumentTable)
            {
                SavingDocument(docInfo.DocCookie);
            }
        }

        public void AltFormatOpenDocuments()
        {
            foreach (var docInfo in _runningDocumentTable)
            {
                ShowingDocument(docInfo.DocCookie);
            }
        }

        public int OnAfterAttributeChange(uint docCookie, uint grfAttribs)
        {
            return VSConstants.S_OK;
        }

        public int OnAfterAttributeChangeEx(uint docCookie, uint grfAttribs, IVsHierarchy pHierOld, uint itemidOld, string pszMkDocumentOld, IVsHierarchy pHierNew, uint itemidNew, string pszMkDocumentNew)
        {
            return VSConstants.S_OK;
        }

        public int OnAfterDocumentWindowHide(uint docCookie, IVsWindowFrame pFrame)
        {
            return VSConstants.S_OK;
        }

        public int OnAfterFirstDocumentLock(uint docCookie, uint dwRDTLockType, uint dwReadLocksRemaining, uint dwEditLocksRemaining)
        {
            return VSConstants.S_OK;
        }

        public int OnBeforeDocumentWindowShow(uint docCookie, int fFirstShow, IVsWindowFrame pFrame)
        {
            if (fFirstShow != 0)
            {
                ShowingDocument(docCookie);
            }
            return VSConstants.S_OK;
        }

        public int OnBeforeLastDocumentUnlock(uint docCookie, uint dwRDTLockType, uint dwReadLocksRemaining, uint dwEditLocksRemaining)
        {
            return VSConstants.S_OK;
        }

        public int OnBeforeSave(uint docCookie)
        {
            SavingDocument(docCookie);
            return VSConstants.S_OK;
        }

        public int OnAfterSave(uint docCookie)
        {
            // If we're not closing the solution, re-run the open formatter
            // (the idea is to always see your alt format if both format on open/save are enabled)
            if (!_isClosing)
            {
                ShowingDocument(docCookie);
            }
            return VSConstants.S_OK;
        }
    }
}
