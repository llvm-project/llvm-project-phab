;;; clang-refactor-rename.el --- Renames symbols found at <offset>.

;; Keywords: tools, c

;;; Commentary:

;; To install clang-refactor-rename.el make sure the directory of this file is
;; in your 'load-path' and add
;;
;;   (require 'clang-refactor-rename)
;;
;; to your .emacs configuration.

;;; Code:

(defcustom clang-refactor "clang-refactor"
  "Path to clang-refactor executable."
  :type 'hook
  :options '(turn-on-auto-fill flyspell-mode)
  :group 'wp)

(defun clang-refactor-rename (new-name)
  "Rename all instances of the symbol at the point using clang-refactor rename"
  (interactive "sEnter a new name: ")
  (let (;; Emacs offset is 1-based.
        (offset (- (point) 1))
        (orig-buf (current-buffer))
        (file-name (buffer-file-name)))

    (let ((rename-command
          (format "bash -f -c '%s -offset=%s -new-name=%s -i %s'"
                               clang-refactor-binary offset new-name file-name)))
          (message (format "Running clang-refactor command %s" rename-command))
          ;; Run clang-refactor via bash.
          (shell-command rename-command)
          ;; Reload buffer.
          (revert-buffer t t)
    )
  )
)

(provide 'clang-refactor)

;;; clang-refactor.el ends here
