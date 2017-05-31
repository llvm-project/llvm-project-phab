using EnvDTE;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System.Linq;
    
namespace LLVM.ClangFormat
{
    // Exposes event sources for IVsRunningDocTableEvents3 events.
    internal sealed class RunningDocTableEventsDispatcher : IVsRunningDocTableEvents3
    {
        private RunningDocumentTable _runningDocumentTable;
        private DTE _dte;
        private SolutionEvents _solutionEvents;
        private bool _isClosing = false;

        public delegate void OnBeforeSaveHander(object sender, Document document);
        public event OnBeforeSaveHander BeforeSave;

        public delegate void OnBeforeShowHander(object sender, Document document);
        public event OnBeforeShowHander BeforeShow;

        private void SolutionIsClosing()
        {
            _isClosing = true;
        }

        public RunningDocTableEventsDispatcher(Package package)
        {
            _runningDocumentTable = new RunningDocumentTable(package);
            _runningDocumentTable.Advise(this);
            _dte = (DTE)Package.GetGlobalService(typeof(DTE));
            _solutionEvents = _dte.Events.SolutionEvents;
            _solutionEvents.BeforeClosing += SolutionIsClosing;
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
                if (BeforeShow != null)
                {
                    var document = FindDocumentByCookie(docCookie);
                    if (document != null) // Not sure why this happens sometimes
                    {
                        BeforeShow(this, document);
                    }
                }
            }
            return VSConstants.S_OK;
        }

        public int OnBeforeLastDocumentUnlock(uint docCookie, uint dwRDTLockType, uint dwReadLocksRemaining, uint dwEditLocksRemaining)
        {
            return VSConstants.S_OK;
        }

        public int OnBeforeSave(uint docCookie)
        {
            if (BeforeSave != null)
            {
                var document = FindDocumentByCookie(docCookie);
                if (document != null) // Not sure why this happens sometimes
                {
                    BeforeSave(this, document);
                }
            }
            return VSConstants.S_OK;
        }

        public int OnAfterSave(uint docCookie)
        {
            // If we're not closing the solution, re-run the open formatter
            // (the idea is to always see your alt format if both format on open/save are enabled)
            if (!_isClosing)
            {
                if (BeforeShow != null)
                {
                    var document = FindDocumentByCookie(docCookie);
                    if (document != null) // Not sure why this happens sometimes
                    {
                        BeforeShow(this, document);
                    }
                }
            }
            return VSConstants.S_OK;
        }

        private Document FindDocumentByCookie(uint docCookie)
        {
            var documentInfo = _runningDocumentTable.GetDocumentInfo(docCookie);
            return _dte.Documents.Cast<Document>().FirstOrDefault(doc => doc.FullName == documentInfo.Moniker);
        }
    }
}
