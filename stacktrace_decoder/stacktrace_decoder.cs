﻿using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using static principia.tools.DbgHelp;

namespace principia {
namespace tools {

class StackTraceDecoder {
  // Returns the base address for the given DLL.
  private static Int64 GetBaseAddress(bool unity_crash,
                                      string unity_regex,
                                      string principia_cpp_regex,
                                      StreamReader stream) {
    var base_address_regex =
        new Regex(unity_crash ? unity_regex
                              : @"^[EI].*" + principia_cpp_regex +
                                    @":.*\] Base address is ([0-9A-F]+)$");
    Match base_address_match;
    do {
      Check(!stream.EndOfStream, $"Could not find base address");
      base_address_match = base_address_regex.Match(stream.ReadLine());
    } while (!base_address_match.Success);
    string base_address_string = base_address_match.Groups[1].ToString();
    return Convert.ToInt64(base_address_string, 16);
  }

  // If the IMAGEHLP_LINEW64 represents a line of Principia code, returns a
  // GitHub link.  Otherwise, returns null.
  // If snippets is true, emits a raw link without Markdown formatting;
  // within the Principia repository, this will be turned into a snippet by
  // GitHub: https://help.github.com/articles/creating-a-permanent-link-to-a-code-snippet/.
  private static string ParseLine(IntPtr handle, IMAGEHLP_LINEW64 line,
                                  SYMBOL_INFOW symbol,
                                  string commit, bool snippets) {
    var file_regex = new Regex(@".*\\[Pp]rincipia\\([a-z_]+)\\(\S+)");
    Match file_match = file_regex.Match(line.FileName);
    if (!file_match.Success) {
      return null;
    }
    string file = $"{file_match.Groups[1]}/{file_match.Groups[2]}";
    bool generated = Regex.IsMatch(file, @"\.generated\.[^/]*$");
    int line_number = line.LineNumber;
    int? start_line_number = line.LineNumber;

    SymGetLineFromAddrW64(
        handle, symbol.Address, out int displacement, line);
    Match symbol_file_match = file_regex.Match(line.FileName);
    if (symbol_file_match.Success &&
        $@"{symbol_file_match.Groups[1]}/{
            symbol_file_match.Groups[2]}" == file &&
        line.LineNumber < line_number) {
      start_line_number = line.LineNumber;
    }

    string url = Uri.EscapeUriString(
        $@"https://github.com/mockingbirdnest/Principia/blob/{commit}/{file}#{
           (start_line_number.HasValue ? $"L{start_line_number}-"
                                       : "")}L{line_number}");
    if (generated) {
      return $"`{file}:{line_number}`";
    } else {
      // Snippets should not be separated by new lines, as they are on their own
      // line anyway, so that a new line spaces them more than necessary.  In
      // order to keep the Markdown readable, hide a new line in a comment.
      // `file:line` links need still to be separated by new lines.
      return snippets ? $"<!---\n--> {url} "
                      : $"\n[`{file}:{line_number}`]({url})";
    }
  }

  private static void Win32Check(bool success,
                                 [CallerMemberName] string member = "",
                                 [CallerFilePath] string file = "",
                                 [CallerLineNumber] int line = 0) {
    if (!success) {
      Console.WriteLine($"Error {Marshal.GetLastWin32Error()}");
      Console.WriteLine($"{file}:{line} ({member})");
      Environment.Exit(1);
    }
  }

  private static void Check(bool condition, string message) {
    if (!condition) {
      Console.WriteLine(message);
      Environment.Exit(1);
    }
  }

  private static string Comment(string comment) {
    // Put the new line in the comment in order to avoid introducing
    // new lines in the Markdown.
    return $"<!---\n {comment} -->";
  }

  private static void Main(string[] args) {
    Int64? base_address_override = null;
    bool unity_crash = false;
    Func<string, string> comment = Comment;
    bool snippets = true;
    string commit = null;

    if (args.Length < 2) {
      PrintUsage();
      return;
    }
    string info_file_uri = args[0];
    string principia_directory = args[1];

    for (int i = 2; i < args.Length; ++i) {
      string flag = args[i];
      var base_address_match = Regex.Match(flag, "--base-address=(0x[0-9A-F]+)");
      var unity_crash_match = Regex.Match(flag, "--unity-crash-at-commit=([0-9a-f]{40})");
      if (!base_address_override.HasValue && base_address_match.Success) {
        base_address_override = Convert.ToInt64(
            base_address_match.Groups[1].ToString(), 16);
      } else if (!unity_crash && unity_crash_match.Success) {
        unity_crash = true;
        commit = unity_crash_match.Groups[1].ToString();
      } else if (snippets && flag == "--no-snippet") {
        snippets = false;
      } else if (comment == Comment && flag == "--no-comment") {
        comment = (_) => "";
      } else {
        Console.WriteLine($"Unrecognized argument {flag}");
        PrintUsage();
        return;
      }
    }

    var web_client = new WebClient();
    var stream = new StreamReader(web_client.OpenRead(info_file_uri),
                                  Encoding.UTF8);
    if (!unity_crash) {
      var version_regex = new Regex(
          @"^[EI].*\] Principia version " +
          @"([0-9]{10}-.+)-[0-9]+-g([0-9a-f]{40})(-dirty)? built");
      Match version_match;
      do {
        Check(!stream.EndOfStream,
              $"Could not find Principia version line in {info_file_uri}");
        version_match = version_regex.Match(stream.ReadLine());
      } while (!version_match.Success);
      commit = version_match.Groups[2].ToString();
      bool dirty = version_match.Groups[3].Success;
      if (dirty) {
        Console.Write(comment(
            $"Warning: version is dirty; line numbers may be incorrect."));
      }
    }
    Int64 principia_base_address = base_address_override ?? GetBaseAddress(
        unity_crash,
        @"(?i)GameData\\Principia\\x64\\principia.dll:principia.dll " +
        @"\(([0-9A-F]+)\)",
        "interface\\.cpp",
        stream);
    Console.Write(
        comment($"Using Principia base address {principia_base_address:X}"));
    var stack_regex = new Regex(
        unity_crash ? @"0x([0-9A-F]+) .*"
                    : @"@\s+[0-9A-F]+\s+.* \[0x([0-9A-F]+)(\+[0-9]+)?\]");
    if (unity_crash) {
      Match stack_start_match;
      do {
        stack_start_match = Regex.Match(
            stream.ReadLine(),
            @"={10} OUTPUTT?ING STACK TRACE ={10}|Stack Trace of Crashed Thread");
      } while (!stack_start_match.Success);
    }
    string log_line;
    Match stack_match;
    do {
      Check(!stream.EndOfStream,
            $"Could not find stack trace in {info_file_uri}");
      log_line = stream.ReadLine();
      stack_match = stack_regex.Match(log_line);
    } while (!stack_match.Success);

    IntPtr handle = new IntPtr(1729);
    SymSetOptions(SYMOPT_LOAD_LINES);
    Win32Check(SymInitializeW(handle, null, fInvadeProcess: false));
    string principia_dll_path =
        Path.Combine(principia_directory, "principia.dll");
    Win32Check(File.Exists(principia_dll_path));
    Win32Check(
        SymLoadModuleExW(handle,
                         IntPtr.Zero,
                         principia_dll_path,
                         null,
                         principia_base_address,
                         0,
                         IntPtr.Zero,
                         0) != 0);

    var trace = new Stack<string>();
    for (;
         stack_match.Success ||
         (unity_crash &&
          !Regex.IsMatch(log_line,
          "={10} END OF STACKTRACE ={10}|Stacks for Running Threads:"));
         log_line = stream.ReadLine(), stack_match = stack_regex.Match(log_line)) {
      if (!stack_match.Success) {
        continue;
      }
      Int64 address = Convert.ToInt64(stack_match.Groups[1].ToString(), 16);
      IMAGEHLP_LINEW64 line = new IMAGEHLP_LINEW64();
      SYMBOL_INFOW symbol = new SYMBOL_INFOW();
      int inline_trace = SymAddrIncludeInlineTrace(handle, address);
      if (inline_trace != 0) {
        Win32Check(SymQueryInlineTrace(handle,
                                       address,
                                       0,
                                       address,
                                       address,
                                       out int current_context,
                                       out int current_frame_index));
        for (int i = 0; i < inline_trace; ++i) {
          Win32Check(SymGetLineFromInlineContextW(
              handle, address, current_context + i, 0, out int dsp, line));
          Win32Check(SymFromInlineContextW(handle,
                                           address,
                                           current_context + i,
                                           out Int64 displacement64,
                                           symbol));
          trace.Push(ParseLine(handle, line, symbol, commit, snippets) ??
                     comment("Inline frame not in Principia code"));
        }
      }
      if (SymGetLineFromAddrW64(handle,
                                address,
                                out int displacement,
                                line)) {
        Win32Check(
            SymFromAddrW(handle, address, out Int64 displacement64, symbol));
        trace.Push(ParseLine(handle, line, symbol, commit, snippets) ??
                   comment($"Not in Principia code: {stack_match.Groups[0]}"));
      } else if (Marshal.GetLastWin32Error() == 126) {
        trace.Push(comment($"Not in loaded modules: {stack_match.Groups[0]}"));
      } else {
        Win32Check(false);
      }
    }
    while (trace.Count > 0) {
      Console.Write(trace.Pop());
    }
  }

  private static void PrintUsage() {
    Console.WriteLine("Usage: stacktrace_decoder " +
                      "<INFO log> <principia_dll_directory> " +
                      "[--no-comment] [--no-snippet]");
    Console.WriteLine("       stacktrace_decoder " +
                      "<Player.log> <principia_dll_directory> " +
                      "--unity-crash-at-commit=<sha1> " +
                      "[--no-comment] [--no-snippet]");
    Console.WriteLine("       stacktrace_decoder " +
                      "<error.log> <principia_dll_directory> " +
                      "--unity-crash-at-commit=<sha1> " +
                      "--base-address=0x<hex> " +
                      "[--no-comment] [--no-snippet]");
  }
}

}  // namespace tools
}  // namespace principia
