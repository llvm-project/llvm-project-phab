//===--- lib/Support/Progress.cpp - ANSI Terminal Progress Bar --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines an ANSI Terminal Progress Bar.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Progress.h"

#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Format.h"

#include <cassert>

using namespace llvm;

void ProgressBar::update(float Percent, StringRef Msg) {
  if (!OS.is_displayed())
    return;

  std::string ETAStr;
  if (ShowETA) {
    if (!Visible)
      Start = std::chrono::system_clock::now();
    auto Elapsed = std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now() - Start).count();
    if (Percent > 0.0001 && Elapsed > 1) {
      auto Total = Elapsed / Percent;
      auto ETA = unsigned(Total - Elapsed);
      unsigned HH = ETA / 3600;
      unsigned MM = (ETA / 60) % 60;
      unsigned SS = ETA % 60;
      raw_string_ostream RSO(ETAStr);
      RSO << format(" ETA %02d:%02d:%02d", HH, MM, SS);
      ETAStr = RSO.str();
    }
  }

  clear();

  unsigned TotalWidth = sys::Process::StandardOutColumns();
  unsigned PBWidth = TotalWidth - (1 + 4 + 2 + 1 + ETAStr.size() + 1);
  unsigned PBDone = PBWidth * Percent;

  // Centered header
  (OS.indent((TotalWidth - Header.size()) / 2)
     .changeColor(HeaderColor, /*Bold=*/true)
     << Header).resetColor() << "\n";

  // Progress bar
  OS << format(" %3d%% ", unsigned(100 * Percent));
  (OS.changeColor(BarColor) << "[").resetColor();
  (OS.changeColor(BarColor, /*Bold=*/true)
     << std::string(PBDone, '=') << std::string(PBWidth - PBDone, '-'))
     .resetColor();
  (OS.changeColor(BarColor) << "]").resetColor();
  OS << ETAStr << "\n";

  // Footer messaage
  if (Msg.size() < TotalWidth) {
    OS << Msg;
    Width = Msg.size();
  } else {
    OS << Msg.substr(0, TotalWidth - 4) << "... ";
    Width = TotalWidth;
  }

  // Hide the cursor
  OS << "\033[?25l";

  OS.flush();
  Visible = true;
}

void ProgressBar::clear() {
  if (!Visible)
    return;

  if (!OS.is_displayed())
    return;

  // Move to beginning of current the line
  OS << "\033[" << (Width + 1) << "D"

     // Clear it
     << "\033[2K"

     // Move up a line and clear it
     << "\033[1A\033[2K"

     // Move up a line and clear it
     << "\033[1A\033[2K"

     // Unhide the cursor
     << "\033[?25h";

  OS.flush();
  Visible = false;
}

