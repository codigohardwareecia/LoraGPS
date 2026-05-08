using GMap.NET;
using GMap.NET.MapProviders;
using GMap.NET.WindowsForms;
using GMap.NET.WindowsForms.Markers;
using System.Globalization;
using System.IO.Ports;
using System.Text.Json;

namespace LoraGPSMonitor
{
    public class Telemetria
    {
        public string Id { get; set; } = "";
        public double Lat { get; set; }
        public double Lng { get; set; }
        public int Sat { get; set; }
        public double Alt { get; set; }
    }

    public partial class Form1 : Form
    {
        private GMapOverlay? markersOverlay;
        private SerialPort? btPort;
        private readonly string cachePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "MapCache");
        private GMarkerGoogle? realTimeMarker;

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            SetupMap();
            IniciarBluetooth();
        }

        private void SetupMap()
        {
            GMap.NET.MapProviders.GMapProvider.UserAgent = "seuemail@hotmail.com";
            System.Net.ServicePointManager.SecurityProtocol = System.Net.SecurityProtocolType.Tls12;
            gmap.Dock = DockStyle.Fill;

            // Configuração de Cache
            if (!Directory.Exists(cachePath)) Directory.CreateDirectory(cachePath);
            gmap.CacheLocation = cachePath;
            gmap.Manager.Mode = AccessMode.ServerAndCache;

            // Provedor e Posicionamento
            gmap.MapProvider = GMapProviders.OpenStreetMap;
            gmap.Position = new PointLatLng(-23.5489, -46.6388); // Ex: São Paulo
            gmap.MinZoom = 2;
            gmap.MaxZoom = 20;
            gmap.Zoom = 15;
            gmap.DragButton = MouseButtons.Left;

            markersOverlay = new GMapOverlay("rastreio");
            gmap.Overlays.Add(markersOverlay);
            this.Controls.Add(gmap);
        }

        private void IniciarBluetooth()
        {
            btPort = new SerialPort("COM16", 9600)
            {
                DtrEnable = true,
                RtsEnable = true,
                ReadTimeout = 5000
            };

            try
            {
                btPort.Open();
                Task.Run(LerDadosLoop); // Loop de leitura ativa (Polling)
                MessageBox.Show("Conectado!");
            }
            catch (Exception ex) { MessageBox.Show(ex.Message); }
        }

        private void LerDadosLoop()
        {
            while (btPort != null && btPort.IsOpen)
            {
                try
                {
                    string json = btPort.ReadLine().Trim();
                    var dados = JsonSerializer.Deserialize<Telemetria>(json,
                                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

                    if (dados != null)
                    {
                        this.BeginInvoke(() => {
                            AtualizarMapa(dados);
                        });
                    }
                }
                catch { }
                Thread.Sleep(100);
            }


        }

        private void AtualizarMapa(Telemetria d)
        {
            PointLatLng novoPonto = new PointLatLng(d.Lat + 1, d.Lng);

            // 2. Verifica se o marcador já existe
            if (realTimeMarker == null)
            {
                // Se não existe, cria pela primeira vez
                realTimeMarker = new GMarkerGoogle(novoPonto, GMarkerGoogleType.red_dot);
                realTimeMarker.ToolTipMode = MarkerTooltipMode.Always; // MANTÉM SEMPRE VISÍVEL
                markersOverlay.Markers.Add(realTimeMarker);
            }
            else
            {
                // 3. Se já existe, apenas move e atualiza o texto (O balão não fecha!)
                realTimeMarker.Position = novoPonto;
            }

            // Atualiza o conteúdo do balão
           // realTimeMarker.ToolTipText = $"ID: {d.Id}\nSats: {d.Sat}\nLat: {d.Lat:F6}\nLng: {d.Lng:F6}";
            realTimeMarker.ToolTipText = $"ID: {d.Id}\nSats: {d.Sat} Alt: {d.Alt}\nLat: {0:F6}\nLng: {0:F6}";
                 // Centraliza o mapa no novo ponto
            gmap.Position = novoPonto;
          
        }
    }



}

