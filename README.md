1) ADC_013x_2 Dosyasında
   - Bu proje, TMS320F2800137 işlemcisini kullanarak analog sinyalleri okuyup dijital sinyallere çevirme (ADC) işlemini gerçekleştirmeyi amaçlamaktadır.
   İşlevler;
      - Analog Sinyal okuma
      - ADC(Analog to Digital Converter)
      - Timer kulanımı

2) ADC_PWM_013x Dosyasında
   -Bu proje, TMS320F2800137 işlemcisini kullanarak analog sinyalleri okuyup dijital sinyallere çevirme (ADC) işlemini gerçekleştirmeyi amaçlamaktadır.
   Dönüştürülen dijital sinyali PWM sinyal yapılandırması yaparak PWM sinyali de üretmektedir.
   İşlevler;
      - Analog Sinyal Okuma
      - ADC(Analog to Digital Converter)
      - PWM Sinyali oluşturma
      - Timer kullanımı

3) PWM_013x Dosyasında
   - Bu proje, TMS320F2800137 işlemcisini kullanarak sabit bir PWM yapılandırması yapıp sabit bir PWM sinyal değeri göndermeyi amaçlamaktadır.
   İşlevler;
      - PWM Sinyali oluşturma
      - Timer kullanımı

4) blink_013x Dosyasında
   - Bu proje, TMS320F2800137 işlemcisini kullanarak sabit bir Dijital sinyal gönderir.
   İşlevler;
      - Dijital çıkış sinyali
    
5) button_blink_timer_2_013x Dosyasında
   - Bu Proje TMS320F2800137 işlemcisini kullanarak Dijital giriş değeri aldığında Dijital Çıkış sinyali vermektedir.
   Bekleme süresini ise delay kullanarak değil timer kullanarak yapmaktadır.
   İşlevler;
      - Dijital giriş sinyali
      - Dijital çıkış sinyali
      - Timer kullanımı
    
6) ADS1231_013x Dosyasında
   Bu proje, TMS320F2800137 işlemcisini kullanarak ADS1231 ADC entegresine dijital saat (clock) sinyali göndermektedir. ADS1231 entegresi, bu sinyale karşılık olarak 24 bitlik bir dijital çıkış (DOUT) sinyali üretir. Projede, her 100 ms'de bir dijital saat sinyali gönderilir ve karşılığında 24 bitlik DOUT sinyali alınır. Bekleme süresi, gecikme (delay) kullanmak yerine timer ile kontrol edilmektedir.
   İşlevler;
      - Dijital giriş sinyali
      - Dijital çıkış sinyali
      - Timer kullanımı
      - ADS1231 ADC Entegresi
      - 
