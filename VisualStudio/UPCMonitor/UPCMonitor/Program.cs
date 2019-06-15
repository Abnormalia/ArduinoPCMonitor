using Newtonsoft.Json;
using OpenHardwareMonitor.Hardware;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UPCMonitor
{
	class Program
	{
		static string ARDPORT  = "COM4";
		static SerialPort port = new SerialPort();

		static int   BPPriceRequest = 0;

		struct PCInfo
		{
			public string PCName;
			public string GPUName;
			public int    CPUMHz;
			public float  TotalRAM;
			public int    CPULoad;
			public int    CPUTemp;
			public int    GPUTemp;
			public float  UsedRAM;
		}

		static PCInfo   pcINFO;
		static Computer comp = new Computer();

		static void UpdateGPUTempreture()
		{
			foreach (var hardware in comp.Hardware)
			{
				if (hardware.HardwareType == HardwareType.GpuAti ||
					hardware.HardwareType == HardwareType.GpuNvidia)
				{
					hardware.Update();

					foreach (var sensor in hardware.Sensors)
					{
						if (sensor.SensorType == SensorType.Temperature)
						{
							pcINFO.GPUTemp = (int)sensor.Value.GetValueOrDefault();
						}
					}
				}
			}
		}

		static void UpdateStaticStats()
		{
			pcINFO.TotalRAM = 0;

			foreach (var hardware in comp.Hardware)
			{
				//CPU
				if (hardware.HardwareType == HardwareType.CPU)
				{
					hardware.Update();

					foreach (var sensor in hardware.Sensors)
					{
						if (sensor.SensorType == SensorType.Clock)
						{
							if (!sensor.Name.ToUpper().Contains("BUS"))
							{
								pcINFO.CPUMHz = (int)sensor.Value.GetValueOrDefault();
							}
						}
					}
				}

				//RAM
				if (hardware.HardwareType == HardwareType.RAM)
				{
					hardware.Update();

					foreach (var sensor in hardware.Sensors)
					{
						if (sensor.Name.ToUpper().Contains("USED") ||
							sensor.Name.ToUpper().Contains("AVAILABLE"))
						{
							pcINFO.TotalRAM += sensor.Value.GetValueOrDefault();
						}
					}
				}
			}
		}

		static string GetHWInfo(HardwareType hType, HardwareType hTyp2 = HardwareType.SuperIO)
		{
			foreach (var hardware in comp.Hardware)
			{
				if (hardware.HardwareType == hType)
				{
					return hardware.Name;
				}

				if (hardware.HardwareType != HardwareType.SuperIO && 
					hardware.HardwareType == hTyp2)
				{
					return hardware.Name;
				}
			}

			return "NONE";
		}

		static void UpdateCPULoad()
		{
			foreach (var hardware in comp.Hardware)
			{
				if (hardware.HardwareType == HardwareType.CPU)
				{
					hardware.Update();

					foreach (var sensor in hardware.Sensors)
					{
						//Console.Write(sensor.Name + " : " + sensor.Value.GetValueOrDefault() + "\n");

						if (sensor.SensorType == SensorType.Load)
						{
							if (sensor.Name.ToUpper().Contains("TOTAL"))
								pcINFO.CPULoad = (int)Math.Round(sensor.Value.GetValueOrDefault());
						}

						if (sensor.SensorType == SensorType.Temperature)
						{
							if (sensor.Name.ToUpper().Contains("PACKAGE"))
								pcINFO.CPUTemp = (int)sensor.Value.GetValueOrDefault();
						}
					}
				}
			}
		}

		static void UpdateRAMUsed()
		{
			foreach (var hardware in comp.Hardware)
			{
				if (hardware.HardwareType == HardwareType.RAM)
				{
					hardware.Update();

					foreach (var sensor in hardware.Sensors)
					{
						if (sensor.SensorType == SensorType.Load)
						{
								pcINFO.UsedRAM = (int)Math.Round(sensor.Value.GetValueOrDefault());
						}
					}
				}
			}
		}

		static void SendDATA(String sDATA, String sPort)
		{
			Console.Write("SENDING : " + sDATA + "\n");

			try
			{
				if (!port.IsOpen)
				{
					port.PortName = sPort;
					port.Open();

					port.Write(sDATA);

					port.Close();
				}
			}
			catch (Exception ex)
			{
				Console.Write("SEND DATA ERROR : " + ex.Message);
			}
		}

		static decimal GetBitcoinPrice()
		{
			string json;

			using (var web = new System.Net.WebClient())
			{
				var url = @"https://api.coindesk.com/v1/bpi/currentprice.json";
				json = web.DownloadString(url);
			}

			dynamic obj		 = JsonConvert.DeserializeObject(json);
			var currentPrice = Convert.ToDecimal(obj.bpi.USD.rate.Value);

			return currentPrice;
		}

		static void PrintPCInfo()
		{
			string txtInfo = String.Format
				(
				"--- PC MONITOR v0.1 --- \n" +
				"CPU : {0}( {1} MHz )\n" +
				"GPU : {2}\n" +
				"RAM : {3} GB\n",
				pcINFO.PCName,
				pcINFO.CPUMHz,
				pcINFO.GPUName,
				Math.Round(pcINFO.TotalRAM)
				);

			Console.Write(txtInfo);
		}

		public static void RealtimeDataSendProcess()
		{
			

			Thread.Sleep(1000);

			PrintPCInfo();
			SendDATA( String.Format("<I,{0},{1},{2},{3}>", 
					 pcINFO.PCName, 
					 pcINFO.CPUMHz, 
					 pcINFO.GPUName, 
					 Math.Round(pcINFO.TotalRAM)),
					 ARDPORT);
					 
			while (true)
			{
				Thread.Sleep(1000);

				UpdateCPULoad();
				UpdateRAMUsed();
				UpdateGPUTempreture();

				//Get Bitcoin price once in every 60 sec
				if (BPPriceRequest == 0)
				{
					//Random rand = new Random();
					//SendDATA(String.Format("<B,{0}>", 8600 + rand.Next(-100, 100)), ARDPORT);
					SendDATA(String.Format("<B,{0}>", (int)GetBitcoinPrice()), ARDPORT);

					BPPriceRequest = 60;

					continue;
				}

				BPPriceRequest--;

				SendDATA(String.Format("<U,{0},{1},{2},{3}>",
					pcINFO.CPULoad,
					pcINFO.CPUTemp,
					pcINFO.GPUTemp,
					pcINFO.UsedRAM),
					ARDPORT);

			}
		}

		static void Main(string[] args)
		{
			if (args.Length > 0)
			{
				if (args[0].ToUpper().Contains("COM"))
				{
					ARDPORT = args[0];
				}
			}

			Console.WriteLine("ARDUINO PORT SET TO : '" + ARDPORT + "'");

			port.Parity    = Parity.None;
			port.StopBits  = StopBits.One;
			port.DataBits  = 8;
			port.Handshake = Handshake.None;
			port.RtsEnable = true;
			port.BaudRate  = 9600;

			comp.CPUEnabled = true;
			comp.GPUEnabled = true;
			comp.RAMEnabled = true;

			comp.Open();

			UpdateStaticStats();

			pcINFO.PCName  = GetHWInfo(HardwareType.CPU);
			pcINFO.GPUName = GetHWInfo(HardwareType.GpuAti, HardwareType.GpuNvidia);

			Thread MainThread = new Thread(new ThreadStart(RealtimeDataSendProcess));
			MainThread.Start();

			do
			{
			} while (Console.ReadKey(true).Key != ConsoleKey.Escape);

			MainThread.Abort();
		}
	}
}
