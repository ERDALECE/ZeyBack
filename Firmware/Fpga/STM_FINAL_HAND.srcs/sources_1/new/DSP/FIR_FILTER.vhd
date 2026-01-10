---------------------- FIRFilter.vhd ---------------------------
--                     Bertan Taþkýn
--                       27.7.2017
--
-- Tip 1 FIR Filtre. Kesim frekanslarý, filtre derecesi, filtre
-- tipi gibi parametreler generic kýsmýndan deðiþtirilebilir.
-- Bütün iþlemler pipeline tabanlý yapýlmýþtýr.
--
-- SamplingFrequency : Örnekleme frekansý (Hz cinsinden)
--
-- Frequency1 : Alçak ve yüksek geçiren filtre için kesim frekansý,
-- bant geçiren ve bant durduran fitre için de 1. kesim frekansý.
-- (Hz cinsinden)
--
-- Frequency2 : Bant geçiren ve bant durduran filtre için 2. kesim
-- frekansý. (Hz cinsinden)
--
-- FilterOrder : Filtre derecesi. Tip 1 filtre olduðundan dolayý
-- çift sayý olmalýdýr. 
--
-- FilterType : Filtre tipi. Lowpass, Highpass, Bandpass ya da 
-- Bandstop olabilir.
--
-- WindowType : Pencere fonksiyonu. Rectangular, Bartlett, Blackman,
-- Hanning ya da Hamming olabilir.
--
-- CoefficientWidth : Katsayý Geniþliði. Ýþlemlerin hassasiyetini
-- ayarlar.
--
-- SignalWidth : Giriþ ve çýkýþ sinyallerinin geniþliði.
--
-- Grup Gecikmesi = FilterOrder/2+ceil(log2(FilterOrder/2+1))+2 
--
-- Grup Gecikmesi otomatik olarak hesaplanýp, not olarak konsola
-- yazdýrýlýr.
--
--
 
 
--Kütüphaneler
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
USE ieee.numeric_std.ALL; 
use ieee.math_real.all;
 
entity FIRFilter is
    generic(SamplingFrequency : real := 50000000.0;            --Örnekleme Frekansý
        Frequency1 : real := 10000000.0;                   --1.Kesim Frekansý
        Frequency2 : real := 20000000.0;                   --2.Kesim Frekansý
        FilterOrder : natural := 80;                       --Filtre Derecesi
        FilterType : string := "Lowpass";                  --Filtre Tipi
        WindowType : string := "Hamming";                  --Pencere Tipi
        CoefficientWidth : natural := 16;                  --Katsayý Geniþliði
        SignalWidth : natural := 8);                       --Sinyal Geniþliði
         
    port(Clk, Enable : in std_logic;                           --Clock ve Enable Giriþleri
        SignalIn : in signed(SignalWidth - 1 downto 0);    --Sinyal Giriþi
        SignalOut : out signed(SignalWidth - 1 downto 0); --Sinyal Çýkýþý
		  ValidOut  : out std_logic); 
--attribute use_dsp48 : string;
--attribute use_dsp48 of FIRFilter : entity is "yes";		
end FIRFilter;
 
architecture Behavioral of FIRFilter is
     
    --Toplama iþleminin kaç adýmda gerçekleþeceðini hesaplamada
    --kullanýlan yararlý fonksiyon
    --logorder(x) = ceil(log2(x)) + 1
    function logorder(order : integer) return integer is
    variable x : integer := order;
    variable y : integer := 0;
    begin      
        while x > 1 loop
            x := x / 2 + x mod 2;
            y := y + 1;
        end loop;
         
        return(y + 1);
    end logorder;
     
    --Katsayýlarýn hesaplandýðý fonksiyon
    function Coefficient(x : integer) return signed is
        --Kesim frekanslarýnýn örnekleme frekansý ile normallize edilmesi
        constant wc1 : real := 2.0 * MATH_PI * Frequency1 / SamplingFrequency;
        constant wc2 : real := 2.0 * MATH_PI * Frequency2 / SamplingFrequency;
         
        constant i : real := real(x - FilterOrder / 2);
        variable sincx : real := 0.0;
        variable coeff : real := 0.0;
        variable coeffint : integer := 0;
    begin
         
        ------- Filtre Katsatýlarýnýn Hesaplanmasý ---------
         
        --Alçak geçiren filtre
        --h(t) = sin(w*t)/(pi*t)
        if FilterType = "Lowpass" then
            if i /= 0.0 then
                coeff := sin(wc1 * i) / (MATH_PI * i);
            else
                coeff := wc1 / MATH_PI;
            end if;
         
        --Yüksek geçiren filtre
        --h(t) = [sin(pi*t) - sin(w*t)]/(pi*t)
        elsif FilterType = "Highpass" then
            if i /= 0.0 then
                coeff := -sin(wc1 * i) / (MATH_PI * i);
            else
                coeff := 1.0 - wc1 / MATH_PI;
            end if;
             
        --Bant geçiren  filtre
        --h(t) = [sin(w2*t)- sin(w1*t)]/(pi*t)
        elsif FilterType = "Bandpass" then
            if i /= 0.0 then
                coeff := (sin(wc2 * i) - sin(wc1 * i)) / (MATH_PI * i);
            else
                coeff := (wc2 - wc1) / MATH_PI;
            end if;
             
        --Bant durduran filtre
        --h(t) = [sin(pi*t) - sin(w2*t) + sin(w1*t)]/(pi*t)
        elsif FilterType = "Bandstop" then
            if i /= 0.0 then
                coeff := (sin(wc1 * i) - sin(wc2 * i)) / (MATH_PI * i);
            else
                coeff := 1.0 + (wc1 - wc2) / MATH_PI;
            end if;
             
        --Geçersiz filtre tipi
        else
            report "Invalid Filter Type" severity error;
        end if;
         
        ------------- Pencere Fonksiyonlarý -----------------
         
        --Rectangular Pencere
        --w(n) = 1
        if WindowType = "Rectangular" then
            coeff := coeff * 1.0;
             
        --Bartlett Pencere
        --w(n) = 1 - 2 * |x - N/2| / N
        elsif WindowType = "Bartlett" then
            coeff := coeff * (1.0 - 2.0 * real(abs(x - FilterOrder / 2)) / real(FilterOrder));
             
        --Blackman Pencere
        --w(n) = 0.42 - 0.5 * cos(2* pi * x / N) + 0.08 * cos(2* pi * x / N)
        elsif WindowType = "Blackman" then
            coeff := coeff * (0.42 - 0.5 * cos(2.0 * MATH_PI * real(x) / real(FilterOrder)) + 0.08 * cos(4.0 * MATH_PI * real(x) / real(FilterOrder)));
         
        --Hanning Pencere
        --w(n) = 0.5 - 0.5 * cos(2 * pi * x / N)
        elsif WindowType = "Hanning" then
            coeff := coeff * (0.5 - 0.5 * cos(2.0 * MATH_PI * real(x) / real(FilterOrder))); 
             
        --Hamming Pencere
        --w(n) = 0.54 - 0.46 * cos(2* pi * x / N)
        elsif WindowType = "Hamming" then
            coeff := coeff * (0.54 - 0.46 * cos(2.0 * MATH_PI * real(x) / real(FilterOrder))); 
         
        --Geçersiz pencere fonksiyonu
        else
            report "Invalid Window Type" severity error;
        end if;
         
        ----- Katsayýnýn floating point'den fixed point'e dönüþtürülmesi -----
         
        --Integer'a dönüþtürme
        coeffint := integer(coeff * real(2**(CoefficientWidth - 1)));
         
        --Signed'a dönüþtürme
        return(to_signed(coeffint, CoefficientWidth));
         
    end Coefficient;
     
    --Toplama iþleminde kullanýlan dizinin boyutu
    constant SumArraySize : integer := logorder(FilterOrder / 2 + 1) - 1;
     
    type SignedTable is array(FilterOrder downto 0) of signed(SignalWidth - 1 downto 0);
    type SignedTable1 is array(FilterOrder / 2 downto 0) of signed(SignalWidth downto 0);   
    type SignedTable2 is array(FilterOrder downto 0) of signed(SignalWidth + CoefficientWidth + logorder(FilterOrder) - 1 downto 0);
    type SignedTable3 is array(SumArraySize downto 0) of SignedTable2;
     
    signal Data : SignedTable := (others=>(others=>'0'));
    signal DataSum : SignedTable1 := (others=>(others=>'0'));
    signal DataSumArray : SignedTable3 := (others=>(others=>(others=>'0')));
	 signal count      : integer range 0 to (FilterOrder + 1) := 0;
         
begin
 
    --Eðer filtre derecesi tek ise hata ver
    assert FilterOrder mod 2 = 0 report "FilterOrder must be even number" severity error;
 
    process(Clk, Enable)
    variable ArraySize : integer := 0;
    begin
         
        --Grup gecikmesi not olarak konsola yazdýrýlýyor
        report "Group Delay = " & integer'image(FilterOrder / 2 + logorder(FilterOrder / 2 + 1) + 1) severity note;
         
        --Yükselen kenar ile iþlemler tetiklenir
        if rising_edge(Clk) and Enable = '1' then
             
            --------------------- Verilerin Kaydýrýlmasý ------------------     
            for i in 0 to FilterOrder - 1 loop
                Data(FilterOrder - i) <= Data(FilterOrder - i - 1);
            end loop;
             
            --Yeni veri diziye aktarýlýr
            Data(0) <= SignalIn;
             
            --Daha az çarpma iþlemi yapmak için ayný sayý ile çarpýlacak
            --verileri topla
            for i in 0 to FilterOrder / 2 loop
                if i /= FilterOrder / 2 then
                    DataSum(i) <=  resize(Data(i), SignalWidth + 1) +  resize(Data(FilterOrder - i), SignalWidth + 1); 
                else
                    DataSum(i) <=  resize(Data(i), SignalWidth + 1);
                end if;
            end loop;
             
            --------------- Verilerin Katsayýlar ile Çarpýlmasý -----------------
            for i in 0 to FilterOrder / 2 loop
                DataSumArray(0)(i) <=  resize(DataSum(i) * Coefficient(i), SignalWidth + CoefficientWidth + logorder(FilterOrder));
            end loop;
             
             
            --------------------- Sonuçlarýn toplanmasý ----------------------          
            --Dizideki eleman sayýsý
            ArraySize := FilterOrder / 2 + 1;
             
            for i in 1 to SumArraySize loop
                for j in 0 to ArraySize / 2 + ArraySize mod 2 - 1 loop
                    --Dizideki elemanlar 2'þer 2'þer toplanarak bir sonraki
                    --diziye aktarýlýr. Dizinin eleman sayýsý tek ise son
                    --eleman bir sonraki diziye direkt aktarýlýr.
                    if 2 * j + 1 > ArraySize - 1 then
                        --Tek kalan eleman
                        DataSumArray(i)(j) <= DataSumArray(i - 1)(2 * j);
                    else
                        DataSumArray(i)(j) <= DataSumArray(i - 1)(2 * j) + DataSumArray(i - 1)(2 * j + 1);
                    end if;
                end loop;
                --Bir sonraki dizideki eleman sayýsý
                ArraySize := ArraySize / 2 + ArraySize mod 2;
            end loop;
             
            --Hesaplanan sonucun kýrpýlarak çýkýþa aktarýlmasý
            if DataSumArray(SumArraySize)(0) > 2**(SignalWidth + CoefficientWidth - 2) - 1 then
                SignalOut <= to_signed(2**(SignalWidth - 1) - 1, SignalWidth);
            elsif DataSumArray(SumArraySize)(0) < -2**(SignalWidth + CoefficientWidth - 2) + 1 then
                SignalOut <= to_signed(-2**(SignalWidth - 1), SignalWidth);
            else
               SignalOut <= DataSumArray(SumArraySize)(0)(SignalWidth + CoefficientWidth - 2 downto CoefficientWidth - 1);
            end if;
				
				if count < FilterOrder/2 then
                    count <= count + 1;
                    ValidOut <= '0';
                else
                   ValidOut <= '1';
                end if;
                 
        end if;
     
    end process;
 
end Behavioral;