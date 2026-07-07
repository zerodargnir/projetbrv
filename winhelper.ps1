Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class Win {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x,int y,int cx,int cy,uint f);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int c);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  public static IntPtr hwnd=IntPtr.Zero;
  static bool Cb(IntPtr h, IntPtr l){ if(!IsWindowVisible(h))return true; var sb=new StringBuilder(256);GetWindowText(h,sb,256); if(sb.ToString().IndexOf("Brave Frontier",StringComparison.OrdinalIgnoreCase)>=0) hwnd=h; return true;}
  public static void Go(){ hwnd=IntPtr.Zero; EnumWindows((h,l)=>Cb(h,l),IntPtr.Zero); if(hwnd==IntPtr.Zero)return; ShowWindow(hwnd,9); SetWindowPos(hwnd,new IntPtr(-1),0,0,1216,941,0x40); SetForegroundWindow(hwnd);}
}
"@ -ErrorAction SilentlyContinue

Add-Type @"
using System;using System.Runtime.InteropServices;
public class Mouse {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint x,uint y,uint d,IntPtr e);
  const uint LD=0x0002, LU=0x0004;
  public static void Swipe(int x1,int y1,int x2,int y2,int steps){
    SetCursorPos(x1,y1); System.Threading.Thread.Sleep(120); mouse_event(LD,0,0,0,IntPtr.Zero); System.Threading.Thread.Sleep(120);
    for(int i=1;i<=steps;i++){ int x=x1+(x2-x1)*i/steps; int y=y1+(y2-y1)*i/steps; SetCursorPos(x,y); System.Threading.Thread.Sleep(15); }
    System.Threading.Thread.Sleep(80); mouse_event(LU,0,0,0,IntPtr.Zero);
  }
}
"@ -ErrorAction SilentlyContinue

function Pin-Game { [Win]::Go(); Start-Sleep -Milliseconds 400 }
# Swipe en fractions de fenetre (1216x941), origine ecran (0,0)
function Swipe-Frac($x1,$y1,$x2,$y2,$steps=25) { [Mouse]::Swipe([int]($x1*1216),[int]($y1*941),[int]($x2*1216),[int]($y2*941),$steps) }
function Shot-Game($path="C:\Users\mewrd\Desktop\brv\_game_win.png") {
  [Win]::Go(); Start-Sleep -Milliseconds 300
  Add-Type -AssemblyName System.Drawing
  $bmp=New-Object System.Drawing.Bitmap(1216,941); $g=[System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen(0,0,0,0,(New-Object System.Drawing.Size(1216,941)))
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose();$bmp.Dispose()
}
