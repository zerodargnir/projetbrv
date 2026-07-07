# Capture la fenetre Brave Frontier (resout sa position) -> _game_win.png
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class Shot {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
  public static IntPtr hwnd=IntPtr.Zero; public static RECT rect;
  static bool Cb(IntPtr h, IntPtr l){ if(!IsWindowVisible(h))return true; var sb=new StringBuilder(256);GetWindowText(h,sb,256); if(sb.ToString().IndexOf("Brave Frontier",StringComparison.OrdinalIgnoreCase)>=0){hwnd=h;GetWindowRect(h,out rect);} return true;}
  public static int[] Find(){ EnumWindows((h,l)=>Cb(h,l),IntPtr.Zero); if(hwnd==IntPtr.Zero) return new int[]{0,0,0,0}; return new int[]{rect.L,rect.T,rect.R-rect.L,rect.B-rect.T}; }
}
"@
$r=[Shot]::Find()
if($r[2] -eq 0){ Write-Output "fenetre introuvable"; exit }
Add-Type -AssemblyName System.Drawing
$bmp=New-Object System.Drawing.Bitmap($r[2],$r[3])
$g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($r[0],$r[1],0,0,(New-Object System.Drawing.Size($r[2],$r[3])))
$bmp.Save("C:\Users\mewrd\Desktop\brv\_game_win.png",[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose();$bmp.Dispose()
$alive=[bool](Get-Process BraveFrontier.Windows -EA SilentlyContinue)
Write-Output "shot ok rect=$($r -join ',') vivant=$alive"
