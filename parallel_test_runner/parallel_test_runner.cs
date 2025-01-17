﻿using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace principia {
namespace parallel_test_runner {

class ParallelTestRunner {
  enum Granularity {
    Package,
    TestCase,
    Test,
  }

  private static T ParseEnum<T>(string value) {
    return (T)Enum.Parse(typeof(T), value, true);
  }

  private const string vsinstr =
      @"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Team Tools" +
      @"\Performance Tools\x64\vsinstr.exe";

  static void StubbornStart(Process process) {
    for (int remaining = 10; remaining > 0; --remaining) {
      try {
        process.Start();
        return;
      } catch (Exception e) {
        Console.WriteLine("Exception " +
                          e +
                          process.StartInfo.FileName +
                          " " +
                          process.StartInfo.Arguments);
        if (remaining == 1) {
          throw;
        }
        Thread.Sleep(TimeSpan.FromSeconds(1));
      }
    }
  }

  static Task RunProcessAsync(string file_name, string args) {
    var process = new Process{StartInfo = {FileName = file_name,
                                           Arguments = args,
                                           UseShellExecute = false,
                                           RedirectStandardError = false,
                                           RedirectStandardOutput = true},
                              EnableRaisingEvents = true};
    return new Task(async () => {
      StubbornStart(process);
      while (!process.StandardOutput.EndOfStream) {
        Console.WriteLine(await process.StandardOutput.ReadLineAsync());
      }
      process.WaitForExit();
      process.Close();
    });
  }

  static void Main(string[] args) {
    Granularity? granularity_option = null;
    bool? instrument_option = null;
    bool? also_run_disabled_tests_option = null;
    string filter_option = null;

    var death_test_processes = new List<Process>();
    var processes = new List<Process>();
    int test_process_counter = 0;

    var instrument_tests = new List<Task>();

    foreach (string arg in args) {
      if (arg.StartsWith("--") && arg.Contains(":")) {
        string[] split = arg.Split(new []{"--", ":"}, StringSplitOptions.None);
        string option = split[1];
        string value = split[2];
        if (option == "granularity") {
          granularity_option = ParseEnum<Granularity>(value);
        } else if (option == "instrument") {
          instrument_option = bool.Parse(value);
        } else if (option == "also_run_disabled_tests") {
          also_run_disabled_tests_option = bool.Parse(value);
        } else if (option == "filter") {
          filter_option = value;
        } else {
          Console.WriteLine("Unknown option " + option);
          Environment.Exit(1);
        }
        continue;
      }
      Granularity granularity = granularity_option ?? Granularity.Test;
      bool instrument = instrument_option ?? false;
      bool also_run_disabled_tests = also_run_disabled_tests_option ?? false;
      string filter = filter_option;
      granularity_option = null;
      instrument_option = null;
      filter_option = null;

      if (filter != null && granularity == Granularity.Package) {
        Console.WriteLine(
            "--filter is not supported with --granularity:Package");
        Environment.Exit(1);
      }

      string[] test_binaries = Directory.GetFiles(arg, "*_tests.exe");
      foreach (string test_binary in test_binaries) {
        if (instrument) {
          instrument_tests.Add(
              RunProcessAsync(vsinstr, "/coverage \"" + test_binary + "\""));
        }
        if (granularity == Granularity.Package) {
          var process = new Process{
              StartInfo = {
                  UseShellExecute = false,
                  RedirectStandardOutput = true,
                  RedirectStandardError = true,
                  FileName = test_binary,
                  Arguments = "--gtest_filter=-*DeathTest.*"
              }
          };
          process.StartInfo.Arguments +=
              " --gtest_output=xml:TestResults\\gtest_results_" +
              test_process_counter++ + ".xml";
          processes.Add(process);
          process = new Process{
              StartInfo = {
                  UseShellExecute = false,
                  RedirectStandardOutput = false,
                  RedirectStandardError = false,
                  FileName = test_binary,
                  Arguments = "--gtest_filter=*DeathTest.*"
              }
          };
          process.StartInfo.Arguments +=
              " --gtest_output=xml:TestResults\\gtest_results_" +
              test_process_counter++ + ".xml";
          if (also_run_disabled_tests) {
            process.StartInfo.Arguments += " --gtest_also_run_disabled_tests";
          }
          death_test_processes.Add(process);
          continue;
        }
        var info = new ProcessStartInfo(
            test_binary,
            filter == null
                ? "--gtest_list_tests"
                : $"--gtest_list_tests --gtest_filter={filter}"){
            UseShellExecute = false,
            RedirectStandardOutput = true
        };
        var list_tests = Process.Start(info);
        var output = list_tests.StandardOutput;
        string test_case = null;
        bool? is_death_test = null;
        while(!output.EndOfStream) {
          string line = output.ReadLine();
          if (line[0] != ' ') {
            test_case = line;
            is_death_test = Regex.Match(line, ".*DeathTest").Success;
            if (granularity == Granularity.TestCase) {
              var process = new Process{
                  StartInfo = {
                      UseShellExecute = false,
                      RedirectStandardOutput = true,
                      RedirectStandardError = true,
                      FileName = test_binary,
                      Arguments = "--gtest_filter=" + test_case + "*"
                  }
              };
              process.StartInfo.Arguments +=
                  " --gtest_output=xml:TestResults\\gtest_results_" +
                  test_process_counter++ + ".xml";
              if (also_run_disabled_tests) {
                process.StartInfo.Arguments +=
                    " --gtest_also_run_disabled_tests";
              }
              if (is_death_test.Value) {
                death_test_processes.Add(process);
              } else {
                processes.Add(process);
              }
            }
          } else if (granularity == Granularity.Test) {
            var process = new Process{
                StartInfo = {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    FileName = test_binary,
                    Arguments = Encoding.Default.GetString(
                        Encoding.UTF8.GetBytes(
                            "--gtest_filter=" + test_case + line.Split(' ')[2]))
                }
            };
            process.StartInfo.Arguments +=
                " --gtest_output=xml:TestResults\\gtest_results_" +
                test_process_counter++ + ".xml";
            if (also_run_disabled_tests) {
              process.StartInfo.Arguments += " --gtest_also_run_disabled_tests";
            }
            if (is_death_test.Value) {
              death_test_processes.Add(process);
            } else {
              processes.Add(process);
            }
          }
        }
      }
    }
    var stopwatch = new Stopwatch();

    Console.WriteLine("Instrumenting " + instrument_tests.Count +
                      " processes...");
    stopwatch.Start();
    if (instrument_tests.Count > 0) {
      instrument_tests.ForEach(task => task.Start());
      Task.WaitAll(instrument_tests.ToArray());
      Console.WriteLine("Done (" + stopwatch.ElapsedMilliseconds + " ms)");
    }

    Console.WriteLine("Running " + death_test_processes.Count +
                      " death test processes...");
    stopwatch.Restart();

    foreach (Process process in death_test_processes) {
      process.StartInfo.RedirectStandardOutput = false;
      process.StartInfo.RedirectStandardError = false;
      StubbornStart(process);
      process.WaitForExit();
      if (process.ExitCode != 0) {
        Console.WriteLine("Exit code " + process.ExitCode +
                          " from a death test");
        Environment.Exit(process.ExitCode);
      }
      process.Close();
    }

    Console.WriteLine("Running " + processes.Count + " processes...");
    var process_semaphore = new Semaphore(initialCount: max_parallelism,
                                          maximumCount: max_parallelism);

    Task[] tasks = new Task[processes.Count];
    var   errors = new ConcurrentBag<string>();
    for (int i = 0; i < processes.Count; ++i) {
      var process = processes[i];
      // We cannot use i in the lambdas, it would be captured by reference.
      int index = i;
      process_semaphore.WaitOne();
      StubbornStart(process);
      tasks[i] = Task.Run(() => {
        Task standard_output_writer = Task.Run(async () => {
          while (!process.StandardOutput.EndOfStream) {
            Console.WriteLine("O" +
                              index.ToString().PadLeft(4) +
                              " " +
                              await process.StandardOutput.ReadLineAsync());
          }
        });
        Task standard_error_writer = Task.Run(async () => {
          while (!process.StandardError.EndOfStream) {
            Console.WriteLine("E" +
                              index.ToString().PadLeft(4) +
                              " " +
                              await process.StandardError.ReadLineAsync());
          }
        });
        process.WaitForExit();
        if (process.ExitCode != 0) {
          errors.Add("Exit code " +
                     process.ExitCode +
                     " from (" +
                     index.ToString() +
                     ") " +
                     process.StartInfo.FileName +
                     " " +
                     process.StartInfo.Arguments);
        }
        Task.WaitAll(new Task[]
                         { standard_output_writer, standard_error_writer });
        process.Close();
        process_semaphore.Release();
      });
    }

    Task.WaitAll(tasks);
    foreach (string error in errors) {
      Console.WriteLine(error);
    }
    Console.WriteLine("Done (" + stopwatch.ElapsedMilliseconds + " ms)");
    Environment.Exit(errors.Count);
  }

  // Maximum number of processes to execute in parallel.
  private static readonly int max_parallelism = 100;
}

}  // namespace parallel_test_runner
}  // namespace principia
