Внимание! Проект пока не готов! Хотя у меня время вполне нормально показывает.

# Очередные часы с будильником, датчиком температуры и давления, на маленькой матрице одноцветных светодиодов.

Основано на моём [прошлом проекте](https://github.com/SerhiiLe/clock-esp8266-ws2812b), но при создании закладывалась совершенно другие цели и другой способ использования. Это небольшие настольные часы-будильник, без резервного питания и если утром электричества не будет, то будильник не разбудит. Естественно, что все "охранные" функции не работают, управление через Telegram стало не нужно. С другой стороны, плата часов с резервным питанием позволяет отображать время сразу после подачи питания и работать без интернет. Так-же добавлены простые функции "погодной станции", хотя точность показателей намного хуже полноценной погодной станции.

## Смысл создания этих часов

Существует бесчисленное множество готовых схем и прошивок для намного более функциональных и интересных часов, достаточно сделать [такой поиск](https://www.google.com/search?q=MAX7219+clock). Например [такие](https://www.thingiverse.com/thing:3924947), которые умеют наверное всё :) Фактически готовое устройство, прошивку для которого не надо компилировать, а обновления прилетают из Интернет. И по сути там вся сложность это сделать корпус. Ещё есть такой [замечательный вариант](https://github.com/IZ76/VZ_Clock) А вообще по поиску можно найти много вариантов, от таких сложных и функциональных, как я привёл, до очень простых, но вполне рабочих.

Для меня это часы "антистресс". С одной стороны это очередная никому не нужная версия часиков, с другой стороны это действительно практичная вещь, а создание как "железа", так и прошивки позволило мне отвлечься от рутины будней. И не дать заплесневеть мозгам. А ещё интересно сравнить, как решил ту или иную проблему я и как её решали другие. Ход мысли у всех очень разный. Не всегда мой вариант лучше.

## Ход создания часов

Далее идут заметки, в которых я просто записываю свои мысли, которы возникают при работе над проектом. Почему я выбрал те или иные решения. И немного сравнение с другими проектами.

В начале я хотел сделать маленький вариант часов из моего [прошлого проекта](https://github.com/SerhiiLe/clock-esp8266-ws2812b), просто выкинув всё лишнее. Всего за день адаптации у меня уже матрица показывала время. Но тогда-же пришло понимание, что так просто не получится.

1. Шрифты которые хорошо работали на большой матрице из адресных светодиодов плохо смотрелись на маленькой матрице с высокой плотностью. Прошлось частично перерисовать шрифты и добавить ещё один "крошечный" шрифт, который абсолютно не читаемый на большой матрице, но вполне сносно выглядит на маленькой. Свои шрифты это дело вкуса. Я не использовал обычную для таких часиков библиотеку [MAX72xx](https://majicdesigns.github.io/MD_MAX72XX/), с её шрифтами или [MD_Parola](https://majicdesigns.github.io/MD_Parola/index.html) с её готовыми эффектами. Только хардкор.

2. Старая, простая схема запуска часов плохо работала и пришлось сделать "отложенный старт", запуск стал стабильным и информативным. Наработки перенесены в старый проект.

3. В старом проекте очень много внимания было уделено цвету, это логично для цветной матрицы. На монохромной матрице это всё не нужно и пошло под нож. Зато появилась возможность выводить инвертировать буквы и писать "черным по белому". Выглядит так себе, зато позволяет сделать меню настройки времени кнопками.

4. Самые большие мучения в старом проекте были связаны с модулем DFPlayer, который умеет играть mp3 с SD карточки. Скорее всего мне попался китайски плагиат китайского чипа, так как если верить интернету, то ни у кого таких как у меня проблем не возникло. С другой стороны, я нашел информацию, что есть как минимум две модификации этого чипа с немного разными командами, но у меня не работали оба варианта. Я нашел способ заставить плату работать обеспечивая музыкальный будильник и это должно работать на любых модификациях платы, но осадок остался. По этому для новых часиков я поставил простую пищалку. У чипа ESP8266 нет аппаратного PWM, только программная имитация через таймер и прерывания. Из-за этого получить чистый тон проблемно, варианты как на Atmega328p (Arduino) не работают. По этому пищалка активная, которая умеет выдавать писк только на одной частоте. "Мелодии" это разные варианты продолжительности писка и пауз.

5. Когда я делал прошлый проект, то просто хотел повторить чужой готовый и ничего не знал ни про варианты плат ни про схемотехнику. Если было написано, что платы NodeMCU это хорошо, а WeMos D1 mini это плохо и есть проблемы с устойчивостью работы, то я в это верил. Практика показала, что зря. WeMos D1 mini отличная плата, она компактная и на ней выведены только те ноги, которые можно использовать, а по устойчивости она ни чем не отличается. Да и при программировании, разница только в таблице соответствия обозначений на плате и в программе. Но если к примеру вместо D0 писать 16, можно выбирать любую плату с чипом esp8266.

6. У esp8266 очень мало свободных входов/выходов, всего 9 цифровых и 1 аналоговый. Ещё два это TX/RX используется для программирования и вывода сообщений отладки. Из этих 9 два I2C и четыре SPI. Остаётся 3, один работает через таймер, а два используются для входа в режим программирования и диагностики запуска, все три нельзя использовать в момент загрузки. По этому, для возможности подключить пищалку и чтобы она не пищала при каждом старте, матрица подключается программным SPI, что не добавляет стабильности, но и не вызывает заметных проблем.

7. ESP8266 или ESP32? Теперь я понимаю, что для прошлого проекта нужна была плата ESP32, больше пинов, в том числе аналоговых позволило бы не просто смотреть наличие +5В, а и измерять напряжение аккумулятора. Два ядра позволило бы вынести работу с WiFi и кнопками на отдельное ядро, что устранило-бы или сильно уменьшило ефект "залипания" или "замерзание" картинки. Второй аппаратный Serial добавил бы стабильность работы с DFPlayer. Поддержка чипом сенсорных входов позволило-бы отказаться от отдельной кнопки и добавить их хотя-бы две, что позволило бы выставлять время с кнопок, а не только через WiFi. Но тогда я этого не знал, а сейчас часы заклеены намертво и разваливать корпус совсем не хочется. С другой стороны для маленьких часиков всё это не нужно, по этому ESP8266 подходит больше из-за компактности. Чипы ESP8266 и ESP32 очень разные не только количеством ядер ножками и памятью, самое главное отличие в том, что ESP8266 имеет свою "OS" - ESP-IDF (Espressif IoT Development Framework), а ESP32 работает на FreeRTOS. Даже если писать на Arduino, то всё равно это различие не позволяет писать 100% переносимый код.

8. В старом проекте конфигурация сохранялась в виде JSON файлов на внутренней LittleFS и выдавалась оттуда напрямую в WEB интерфейс. Основной риск в том, что число циклов записи на флеш по паспорту всего 10тыс. На плате часов HW-111 (DS1307) установлен EEPROM объёмом 4096 байт и ресурсом циклов записи 1 миллион. Просто грех не использовать его. Я разработал простейший вариант хранения настроек в виде областей с контрольной суммой. Минус в том, что любое изменение структуры делает старый конфиг не читаемым и происходит новая инициализация. С другой стороны такое хранение намного проще, чем JSON и нет проблем с сохранением настроек, с которым я столкнулся пытаясь повторить проект больших часиков и это было одной из причин начать делать свой.

9. При написании старого проекта у меня был вопрос, как сделать из статичной странички динамическую и приемлемо выглядящую как на большом мониторе, так и на телефоне. Динамика создаётся javascript и JSON. А оформление я сделал на основе таблиц. Как показала практика, таблицы это относительно быстро и просто, но есть ограничения. По этому я переосмыслил интерфейс и переписал таблицы в flex. Практически то-же, но отображение стало более гибким. Был вариант использовать готовый конструктор вроде [GyverPortal](https://github.com/GyverLibs/GyverHub), или [JeeUI](https://github.com/jeecrypt), или [WebApp Builder](https://wicard.net/projects/Arduino/ESP32-ESP8266/esp8266-webapp-builder), или [EmbUI](https://github.com/vortigont/EmbUI) возможно было бы лучше, но как-же тогда свой велосипед?

10. Плата часов HW-111 как и многие другие предназначены под установку аккумулятора, а не батареи, там есть цепь заряда, а это убивает обычные батарейки. Впрочем как говорит интернет и аккумуляторы тоже долго не живут. Для решения проблемы надо переставить диод на плате, как сделал я. Но лучше выпаять вообще отсек батареи и припаять ионистор (суперконденсатор). Ионистор вечный, по сравнению с батарейками или аккумуляторами и заряд будет держать минимум месяц. [Так выглядит переставленный диод у меня.](https://github.com/SerhiiLe/Clock_Mini/blob/main/PXL_20240131_123252839.jpg)

11. Вывод крошечным шрифтом - полная лажа. Как и всякие эффекты. Часы 90% времени должны просто и максимально скучно показывать время. Это их главное предназначение. И вообще, часто что-то делаешь, ломаешь голову, а потом оказывается, что это не нужно. И дилема, удалить или оставить.

12. В плане веб-дизайна, каждый броузер имеет свои листы стилей по умолчанию и они километровые. Чтобы страничка смотрелась примерно одинаково во всех броузерах надо писать свои такие-же километровые листы стилей. Или подключать какой-нибудь готовый, как bootstrap. Средняя современная страничка это условный bootstrap + jquery + react + перегруженная вёрстка для обеспечения работы всего этого счастья и вот имеем страничку чуть сложнее чем hello world объёмом 5Мбайт. И такие объёмы требуют скоростных каналов, "примитивный" 3G уже не подходит. 4G тормозит. 5G пока тянет, но срочно нужен 6G... А потом эта страничка развернётся броузером и после компиляции и рендеринга займёт 200Мб памяти. Вот так и живём.

13. При работе с EEPROM обнаружилась неприятная особенность, а именно скорость работы. Запись происходит блоками до 32 байт, после чего нужно чипу давать время на само запоминание, это примерно 20мсек. Полный объём EEPROM 4 кБайт, или 4096/32x0.02=2.56 сек. Если дробить запись на условные int16 то это 4096/2x0.02=40.96 сек. В итоге - писать по пару байт совсем не выгодно, со стороны пользователя это выглядит как зависание железки. Только блочная запись небольшими блоками.

14. Решил поставить готовые прошивки и посмотреть, как реально работают другие велосипеды. 
Два которые я привёл в самом начале самые сложные и функциональные из всего, что я нашел. Почти все прошивки уровня новичка, как-то показывает время и температуру и счастье. Эти две технологически намного сложнее и меня реально удивили некоторые решения. Особенно когда я увидел, что прошивка на IDF вместо Arduino/IDF позволяет избавится от мусора на ножках и полноценно использовать 12 GPIO (к одному надо подпаиваться напрямую к модулю ESP12). Ещё меня удивил подход подключения DFPlayer без обратной связи, кинул команду, а выполнится или нет никого не волнует. Обе прошивки сделаны как универсальные, все драйвера уже внутри и не надо ничего заранее конфигурировать, достаточно через web включить или выключить нужный датчик или периферию.
[Первое место](https://www.thingiverse.com/thing:3924947) за большую стабильность, удобный web и возможность подключить DFPlayer. Профессиональный продукт.
[Второе место](https://github.com/IZ76/VZ_Clock) за нестабильную работу, неадаптированный под смартфон web и изнасилование flash микроконтроллера, есть риск долго не прожить. Зато там есть большой плюс в открытых исходниках и намного лучшей проработки циферблатов.
А я буду продолжать своё. Похоже пик создания часиков всех видов захлестнул планету примерно в 2017 году и практически сошел на нет в 2021.
[Просто бімба :)](https://github.com/bogd/esp32-morphing-clock)

15. Внезапно оказалось, что BearSSL, с помощью которого работает https не поддерживает все варианты шифрования. Процессор esp8266 слишком слабый. Оставлены только самые простые методы. Не все сайты хотят работать со слабым шифрованием. Некоторые сервисы, которые были в планах, отпали. Кроме того запрос http происходит в разы быстрее, чем https. Быстрее, это примерно в 26 раз, в среднем 186 мсек против до 5 сек.

16. Хотя в планы это не входило, но после знакомства с другими проектами сделал несколько циферблатов. Без эффектов, без миллиона шрифтов, только то, чем бы сам хотел пользоваться. Количество костылей на квадратный метр кода зашкаливает. Для двух циферблатов сделал разные варианты мигающего двоеточия. В вариантах циферблатов с секундами ничего не мигает. На мой взгляд мигание это индикация, что часы работают. И дань традиции, когда часы тикали. И ещё наблюдение за чем-то ритмично меняющимся это вариант медитации. Если есть секунды, то именно они являются притягивающим внимание элементом, а дополнительные мигания только сбивают внимание.

17. Почему у меня нет эффектов? Ещё во время прошлого проекта я решил, что мне достаточно будет только бегущей строки, в том числе для циферблата, который просто строка, а не рисуется по одной цифре. Вся логика разбита на небольшие, самостоятельные, логически законченные блоки, каждый из которых может попросить отрисовать на экране. При этом отрисовка не их проблема. Так удобно писать, отлаживать, стабильно работает, но нет преемственности, каждый занят собой и не знает, что там было на экране до него. А для корректной отрисовки эффектов нужно иметь старую картинку и новую и делать какой-то переход между ними. Для этого нужно иметь несколько буферов и реализовать виртуальный экран, и диспетчер, который будет смешивать картинки. Работы много, а стоит ли игра свеч? Ради эффекта который будет длится от 0.2 до 1 секунды. Если относится железке не как к часам, а как к рекламной вывеске - информатору, то эффекты однозначно нужны, так как это привлекает внимание. Но матрица высотой в 3.5 сантиметра в качестве рекламной вывески не очень подходит. На мой взгляд. Можно нагородить многоэтажную сборку, но дешевле купить специальные модули для сборки бегущих строк или готовые собранные бегущие строки.

18. Когда начинаешь делать что-то новое, то вначале планы не очень большие. Иногда они в процессе усыхают, по разным причинам. Или не правильно оценил объём работ, или оказалось, что эта штука вообще не нужна, или нужна, но схема использования на практике другая. Иногда, по этим же причинам, проект наоборот сильно раздувается относительно начальных планов. В этом разрезе интересно было смотреть на эволюцию проектов часиков на MAX7219. Сначала это было простейшее на arduino Uno или Nano + 4 модуля. Потом модулей стало больше в длину, 6, 8. Затем появился второй этаж, третий, четвёртый :) Так из маленьких просто часиков выросли монстры-комбайны. А надо ли? И ещё, чем больше модулей, тем сложнее, чисто конструктивно, обеспечить ровную геометрию и жесткость.

19. Несмотря на то, что написал выше, всё равно сделал то, что не планировал. Кроме циферблатов, мне понравилась идея тянуть всякие фразы из Интернета. Но проблема всех интернет сервисов в том, что они могут меняться или исчезать, по этому прикрутил такой себе конструктор, который позволит с большой вероятностью перестроится на другой сервер без изменения кода. Есть ограничение по протоколам http/https. Вариант с выбором из настроек мне не нравится, слишком большой расход памяти. По этому надо выбирать опцией при сборке.

20. Есть два варианта писать прошивки для микроконтроллеров. Один, встречается в большинстве скетчей и почти в 100% в примерах - вся конфигурация настраивается до компиляции. Делать меню настроек очень скучно, интереснее сосредоточится на алгоритмах, а все настройки в коде. Второй вариант - всё настраивается уже во время работы. Второй вроде бы лучше, хоть и сложнее для разработчика, но можно выкладывать готовые бинарные файлы и тому, кто хочет повторить даже не надо ничего компилировать. Я наверное злой, но это всё DIY устройства, что как-бы намекает на то, что тот, кто хочет повторить должен получить удовольствие не только от работы паяльником и ножовкой. Весь смысл DIY в получении удовольствия от решения относительно простых задач, как бы они не казались сложными с первого взгляда. Дорогу осилит идущий. На мой взгляд оптимальным является вариант, когда конфигурация железа описана при компиляции, а остальное в настройках. 

21. Пару слов про разные модули, они же железки. Например часики, я использовал самый дешевый и не любимый в интернете модуль HW-111 (DS1307 + AT24C32). Для меня он состоит из сплошных плюсов. Стоит копейки, потребляет всего ничего, имеет на борту для вида памяти - 4кб EEPROM (NVRAM) и SRAM на 56 байт. SRAM вроде небольшая по размеру и питается от батарейки, зато имеет неограниченный ресурс записи. Минус в том, что модуль надо перепаивать и обязательно переставлять диод, потому, что напряжение полностью заряженной батарейки CR2032 3.6В, а для питания платы надо не больше 3.4V. Перестановка диода даёт нужное падение напряжения. Ну и говорят, что точность хода не очень. Для часов, которые постоянно синхронизируются с интернет возможный уход на 1-2 сек за месяц... Более распространённый модуль HW-084 (DS3231M + AT24C32). Пишут, что он более точный. 4кб EEPROM присутствует, а вот вместо SRAM теперь два специализированных блока для хранения будильников... Если настроить правильно, то на одной ножке будет 1, что к примеру может будить контроллер. Но для часов это бесполезная штука, они работают постоянно. Перепаивать тоже надо, вместо перестановки диода, можно его или резистор выпаять, эта микросхема питается более высоким напряжением, шаманство с диодом не нужно. Модуль "DS3231M For PI" уже не имеет ни EEPROM ни SRAM. Зато сразу стоит ионистор и ничего не надо перепаивать. Как для меня, то HW-111 самый интересный, при этом и самый дешевый и имел я в ввиду мнения в интернете.

22. Модули для измерения давления, влажности, температуры. DS18B20, DHT11, DHT21 (AM2120, AM2301), DHT22 - все они работают по протоколу 1-wire и питании +5V, не проверял, но похоже можно целую вязанку подключить к одной ножке. Преимущества в том, что их можно вытянуть на несколько метров от микроконтроллера. Но для компактных устройств типа часов это сомнительное преимущество. Вот для климат-контроля в серверной или теплице им более логично находится. BMP180, AHT10, BME280 - 5V и I2C шина. HTU21, BMP280, BMP280+AHT20 - 3.3V и I2C шина. I2C общие с модулем часов. Но важно не перепутать и не подать 5V на модуль 3.3V. Можно их питать с преобразователя 3.3V на микроконтроллере. Насколько я понял, они все 3.3V, но в тех которые 5V стоит свой линейный стабилизатор и реальный диапазон питания от 3.6V до 6V, что на мой взгляд даёт преимущества при стабильном питании от сети, а датчики 3.3V есть смысл использовать только для внешних датчиков на батарейках. Я это всё к тому, что датчики надо выбирать не по принципу, чем дороже - тем лучше, а от задачи. И внимательно читать какое питание :) А с точки зрения софта, они почти не отличаются. Подключил нужный драйвер и получил свои температуру, давление, влажность, смотря что даёт датчик.

23. Ещё одно лирическое отступление. Решил попробовать stm32 и ... Скажем так был весьма удивлён, при чём как в лучшую, так и в худшую сторону. Когда-то, 3 года назад, на работе возникла необходимость установить на нескольких объектах watchdog. Он должен был быть простым, но логика не совсем стандартная и готовых железок по приемлемой цене не оказалось. Но зато рядом оказался магазин, который продаёт наборы Arduino. После небольших дебатов, фирма остановилась на моём варианте устройства и вот уже через час у меня на столе набор пакетиков. Я уже был морально готов закопаться в литературу, потратить пару дней на изучение дивной штуки под названием микроконтроллер. Но по факту всё оказалось настолько просто, что уже через пол часа у меня на столе стоял и щёлкал рэле работающий прототип. Конечно, не всё так просто, на сборку готовых устройств ушло два дня, это разработка и изготовление платы и много работы паяльником и напильником. Но эта простота и скорость с которой можно создавать прототипы, многообразие библиотек, дружелюбность сообщества, доступность документации на любом языке, оптимальные шаблоны использования C++... Так вот, в stm32 ничего этого нет. Я потратил примерно 3 часа, чтобы у меня начал моргать светодиод. И ещё 5 часов, чтобы понять, почему иногда после заливки прошивки плата перестаёт видится, хотя я вижу, что прошивка внутри работает как мне надо. С удивление узнал, что в отличии от Atmega, которая всегда прошивается по SPI и не требует загрузчика вообще, или esp32/esp8266, где загрузчик находится в ROM и это всегда позволяет прошивать через UART, то у stm32 нет загрузчика в принципе. Хочешь прошивать через ST-Link (i2c), ставь загрузчик под i2c, хочешь через UART, ставь другой загрузчик, а если надумал прошивать по USB/UART то ставь... Ну понятно. Так мне было это ни фига не понятно и почему после прошивки, в которую я по наивности не включал загрузчик, а он должен быть частью прошивки, плата терялась. И нигде я не нашёл этой информации. Вот такая хорошая документация. В общем-то, если устройства идут как готовые решения и не планируется менять прошивку, то загрузчик не нужен и можно сэкономить целый килобайт. Есть библиотеки под базовую периферию, но не более и всё на голом C, всё на низком уровне. К примеру digitalWrite(13,1) в ардуино выглядит как LL_GPIO_SetOutputPin((0x40000000UL+0x00010000UL)+0x00001000UL,((0x1UL<<(13U))<<8U)|0x04000020U) в stm32. На самом деле, примерно так всё выглядит и в Arduino, под капотом, чтобы никого не пугать и не заставлять начинающих считать на сколько бит надо что и куда сдвинуть перед записью в регистр, перед этим долго копаясь в документации, где вообще найти этот регистр. С другой стороны меня удивила производительность и цена. Если производство планирует перейти в серию, то stm32 стоит того, чтобы немного помучаться.

24. Как всегда, после того, как что-то новое начало получаться и удовольствие от этого получено, приходит "похмелье", что-то тут не так. Если брать характеристики на бумаге, то модуль на stm32f103c6t6 (название сразу намекает, что новичкам тут не место) во всём лучше, чем Atmega8. Но нет. Не всё так гладко. Почти всё делается на низком уровне и програмно, все протоколы, каждый шаг. В результате прошивка растёт как на дрожжах, а памяти мало. А если взять более высокий уровень, то ESP32 в разных вариантах, с учётом цены, просто унижают stm32 аналогичных по характеристикам серий. А ведь есть ещё RP2040, у которого есть свои плюшки, а прошивку можно писать даже на Python, памяти хватает. У stm32, которую я купил, после активации встроенного USB из 32к осталось 2к под прошивку и делай что хочешь. И толку от того, что этот USB есть, если на саму логику места не остаётся? А есть ещё великие и ужасные микроконтроллеры PIC, в которых самое ужасное это цена программатора, которая напрочь отбивает желание их пробовать.
Итог такой: esp8266 это самый выгодный по цене (~110грн) сейчас микроконтроллер, но он уже прошлое. Его прямая замена esp32-c3 сейчас проходит путь падения цены и уже можно взять примерно за 160грн. Если бы я об этом задумался раньше, то делал бы этот проект на нём. А stm32 это решение для узкого круга задач.

25. Сделал вполне рабочий порт на esp32. К моему удивлению, это потребовало не так много вмешательства в код. Конечно надо ещё разбросать задачи по разным ядрам, но это будет в конце. Самым сложным оказалось подружить esp32 с LittleFS, у меня это не получилось, хотя тесты показывают, что система рабочая, но залить образ диска и залить по ftp не получилось. А вот с SPIFFS всё работает. И ещё меня очень удивило, что один и тот-же код под esp8266 занимает в два раза меньше места, чем на esp32. На esp8266 надо экономить RAM, на esp32 надо экономить Flash. За одно попробовал сделать профиль для esp32-c3, но там совсем по другому расположены выводы.

26. При работе на маленькой яркости наблюдал паразитную засветку некоторых светодиодов. Решил, что это помехи из-за ШИМ светодиодов, добавил небольшой электролитический конденсатор в конце матрицы, помогло. И между платой матрицы и модулями матрицы приклеил доплеровский датчик движения. Он работает прямо через матрицу, его не видно, габарит не добавляет. Изменения в схему внёс, скорее всего больше меняться не будет. [Распиновка esp32-d1-mini тут](https://templates.blakadder.com/wemos_D1_Mini_ESP32.html), отдельную схему не рисовал.

27. Внезапно оказалось, что по цене esp8266 (wemos d1 mini) можно взять esp32-s2 mini. По габаритам он точно совпадает с wemos d1 mini, но по выводам - нет. i2c находится в другом месте. Переназначить пины не проблема, но мне не на чем тестировать. Как внезапно выяснилось, все esp32-xx это одна платформа и адаптация заключается только в правильном назначении выводов в include/defaults.h и указать нужный процессор в platformio.ini , например board_build.mcu = esp32c3

28. В процессе адаптации под esp32 оказалось, что платформа esp8266 не так уж и плоха, память расходуется экономнее, библиотек море, всё вылизано как яйца у кота. Рано списывать этот чип.

29. FTP на ESP32 запустил, на LittleFS. Оказалось, что LittleFS на ESP32 вполне прилично работает, надо было только подправить файл ОТА обновления прошивки. Скопировал системный файл к себе и сделал нужные мне правки.

30. Решил добавить циферблаты этого проекта к своему старому проекту, окирпичил часики, пришлось разламывать корпус. Забыл перед этим вытащить microSD карточку, сломал её пополам, получил депрессию по этому поводу. А причина банальна - надо проверять входные данные и ловить, в данном случае, нулевые указатели. Попал на ровном месте на 250грн, за новую карточку :(

31. Принесли почистить ноутбук. На Celeron 3350. Я понял, что есть вещи хуже, чем сломать microSD. Это поставить на такой ноутбук Windows10. Таких тормозов я давно не видел. 90% процессора кушают собственные процессы Windows, она одновременно хочет обновляться, проверяться на вирусы, под предлогом связи с телефоном что-то собирает на диске, хотя эту опцию не активировали, что-то собирает телеметрия, OneDrive лезет с предложениями зарегистрироваться, а лента новостей плодит в фоне десяток edge, которые добивают систему. И это при том, что для Microsoft Украины не существует и по их мнению Россия должна решить это недоразумение. Впрочем Google хоть не столь категоричен, но тоже считает, что Украина это часть России. На фоне этого блядства установленный на этом же ноутбуке LinuxMint просто летает и Украину признаёт. И это при том, что стоит "тяжелая" версия с cinnamon. Как оказалось, по этой причине хозяева ноутбука пользуются именно Linux. В основном фильмы смотрят.

32. Долго ловил такой глюк: первая синхронизация по NTP сначала переводит часы на два часа вперёд (мой часовой пояс), а через пару секунд возвращает назад. Это сбивало настройку времени в DS1307. Причину не нашел, просто увеличил до 10 секунд ожидание синхронизации. С удивлением обнаружил, что время в микроконтроллере это содержимое счётчика micros64() + смещение относительно 1970 года.

33. Внезапно, если часики подключены по usb к компьютеру с windows, то все устройства на i2c шине перестают работать. Под linux такое не наблюдается. Не понимаю причины такого поведения, ведь usb это всего-лишь конвертор на UART и как он может так странно влиять для меня загадка.

34. Заканчивать проект всегда тяжело. Когда почти всё уже работает то вдруг резко вспоминаются другие проекты, наваливаются другие работы. Плюс ещё москали со своими ракетами портят жизнь и электричество. И тут уже часики отходят на десятый план, а важнее становятся разные системы бесперебойного питания. Отсюда мораль - проект нельзя растягивать, иначе произойдёт выгорание.

35. Возвращаться к проекту через некоторое время, после других проектов не так и просто. Выручают комментарии, на них нельзя экономить, хоть иногда лень их писать.

36. Сделал "ночной режим", когда матрица гаснет ночью и просыпается только если сработал датчик движения. Или просто выключать экран по времени. Прошивка по прежнему не закончена, вместо многих функций заглушки.

37. Мои проекты "часики", это из разряда "можем повторить". Первые, настенные, часики это попытка повторить функционал моего старого телефона с определителем номера. (вот это поворот!) Последние годы он использовался только как часы с будильником, а после отказа от стационарного телефона осталась привычка. Этот проект это попытка повторить хоть чуть-чуть функционал старой дешевой погодной станции, которая до сих пор работает и достаточно точно показывает погоду без интернета. Уже кучу всего накодил, а именно этот функционал ещё нет. Но я подбираюсь всё ближе, хотя функционал будет меньше. Когда пытаешься что-то повторить понимаешь сколько труда может быть вложено в казалось бы простые вещи.

38. Опять педики с болот возбудились, в результате опять сижу по пол дня в каменном веке. И возникла очень странная проблема, связанная с тем, что стандартная функция configTime() ведёт себя совершенно безобразно. Если получить время с NTP не получается, то время переводится просто на сдвиг временной зоны относительно текущего. Получаеться ситуация, что часы получают правильное время с модуля RTC, затем идёт попытка синхронизации с NTP, ответ не приходит и время просто сдвигается на пару часов вперёд (у меня TZ+2). Код был написан так, что то затем время подводилось в RTC, и при следующей перезагрузке сдвигалось ещё раз... Пришлось срочно спионерить откуда-то код синхронизации NTP "ручками". Работает отлично и без сюрпризов. За одно узнал, что оператор Kyivstar не пропускает пакеты NTP. Вот почему?

39. Разобрался с configTime(). Время хранится как micros64() со старта системы + смещение относительно 1970 года + TZ. Если ответ от ntp не приходит configTime() может сделать три запроса после чего считает, что смещать ничего не надо и всё правильно. Смещение TZ устанавливается только при первом обращении. Если первым выставить время от RTC, то сначала configTime() по любому добавит TZ, при первом обращении, а затем корректирует то, что получилось по ответу NTP. А если его нет, то время так и остаётся сдвинутым на TZ. Есть два решения: сначала вызывать configTime(), а только потом смотреть RTC, но тогда пропадает смысл в RTC, которое должно обеспечивать быстрый старт. Или как в итоге сделал я - отказаться от configTime() и сделать прямой запрос NTP.

40. Померял, сколько же потребляют эти часики и был удивлён довольно высоким потреблением - 100 мА при погашеной матрице и от 150 до 350 мА при обычной работе. Результаты примерно одинаковы, как на esp8266, так и на esp32. Интересно, можно ли как-то уменьшить потребление, ведь надо и WiFi опрашивать и экран обновлять...

41. Хоть и писал, что не буду делать циферблаты, но опять их переделал. Раз уже добавил два дополнительных варианта вывода циферблата, то почему бы не наделать разных вариантов. Одни хорошо смотряться на маленьком расстоянии, но плохо читаются на большом. Другие наоборот. Надо подбирать под конкретное место. Добавил вариант отображения 12/24 формата. Пусть будет.

## Что ещё надо сделать

1. Автономный краткосрочный прогноз погоды
2. ~~Отображение текущей погоды~~
3. ~~Порт на esp32 (ориентир по пинам Wemos D1 Mini ESP32) (частично сделано)~~
4. Возможность в настройках выбора места сохранения настроек
5. Придумать какой-то приемлемый вариант работы на разных языках

