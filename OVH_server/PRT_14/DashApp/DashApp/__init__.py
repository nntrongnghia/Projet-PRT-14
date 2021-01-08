# -*- coding: utf-8 -*-

# Run this app with `python app.py` and
# visit http://127.0.0.1:8050/ in your web browser.

# ======== import
import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output
import plotly.express as px
import pandas as pd
import sqlite3 as sql
from datetime import datetime as dt
import pytz
import time
from scipy import integrate
from calendar import monthrange
from flask import g

# ======= Instances
app = dash.Dash(__name__)
app.config.update({'requests_pathname_prefix': '/dashboard/'})
server = app.server
dbPath = "/home/ubuntu/PRT_14/DashApp/DashApp/database/demo_PRT14.db"
# conn = sql.connect(dbPath, check_same_thread=False)
# cur = conn.cursor()


def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sql.connect(dbPath, check_same_thread=False)
    return db


@server.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

# ========== Web component


def recent_measure():
    # query = "select * from puissance_table ORDER by epoch_time DESC limit 1"
    # result = cur.execute(query).fetchone()  # (epoch, puissance)
    # with server.app_context():
    #     result = get_db().cursor().execute(query).fetchone()  # (epoch, puissance)
    # date = dt.fromtimestamp(result[0], pytz.timezone('Europe/Paris'))
    return html.Div([
        html.H1("Mesure récente"),
        html.Div(className="big-number", id="big-number"),
        html.Div(id="recent-measure-time")
        # html.Div("{:.0f} W".format(result[1]), className="big-number"),
        # html.Div(date.strftime("%d/%m/%Y - %H:%M"), id="recent-measure-time")
    ], className="Recent-Measure")


def hourly_measure():
    # obtenir la plage des dates
    query = "select epoch_time from puissance_table ORDER by epoch_time ASC"
    # list of tuples with only epoch time
    # result = cur.execute(query).fetchall()
    with server.app_context():
        result = get_db().cursor().execute(query).fetchall()
    min_date = dt.fromtimestamp(result[0][0], pytz.timezone('Europe/Paris')).date()
    max_date = dt.fromtimestamp(result[-1][0], pytz.timezone('Europe/Paris')).date()

    day_picker = dcc.DatePickerSingle(
        id='hourly-dcc-date-picker',
        min_date_allowed=min_date,
        max_date_allowed=max_date,
        initial_visible_month=max_date,
        date=max_date,
        display_format='DD/MM/YYYY'
    )
    main_div = html.Div([
        html.Div(day_picker, className="date-selector"),
        html.Div(
            dcc.Graph(id="hourly-plot"),
            className="hourly-plot")
    ], className="Hourly-Measure")
    return main_div


def monthly_energy():
    main_div = html.Div([
        html.H1(className="monthly-title", id="monthly-title"),
        html.Div(id="monthly-energy-text",
                 className="monthly-energy-text big-number")
    ], className="Monthly-Energy")
    return main_div


def daily_energy():
    # obtenir la plage des dates
    query = "select epoch_time from puissance_table ORDER by epoch_time ASC"
    # list of tuples with only epoch time
    with server.app_context():
        result = get_db().cursor().execute(query).fetchall()
    # result = cur.execute(query).fetchall()
    min_date = dt.fromtimestamp(result[0][0], pytz.timezone('Europe/Paris')).date()
    max_date = dt.fromtimestamp(result[-1][0], pytz.timezone('Europe/Paris')).date()
    month_picker = dcc.DatePickerSingle(
        id='daily-energy-month-picker',
        min_date_allowed=min_date,
        max_date_allowed=max_date,
        initial_visible_month=max_date,
        date=max_date,
        display_format='MM/YYYY'
    )
    main_div = html.Div([
        month_picker,
        dcc.Graph(id="daily-energy-plot")
    ], className="Daily-Energy")
    return main_div


#=============================================
# ========== Web components' callbacks
@app.callback(
    [Output('hourly-plot', 'figure'),
     Output('big-number', 'children'),
     Output('recent-measure-time', 'children')],
    [Input('hourly-dcc-date-picker', 'date')])
def update_hourly_plot(date):
    timezone = pytz.timezone('Europe/Paris')
    #date is initially in str type
    datetimeOjb = dt.strptime(date, "%Y-%m-%d").astimezone(timezone)
    min_epoch = time.mktime(datetimeOjb.timetuple())
    max_epoch = min_epoch + 24*60*60
    query = "select * from puissance_table where epoch_time BETWEEN {} and {}".format(
        min_epoch, max_epoch)
    with server.app_context():
        df = pd.read_sql_query(query, get_db())
    # df = pd.read_sql_query(query, conn)
    # work around for the time zone problem
    # add the timedelta to epoch_time
    df.epoch_time = pd.to_datetime(df.epoch_time, unit='s') + timezone.utcoffset(dt.utcnow())
    fig = px.line(df, x="epoch_time", y="puissance",
                  labels={
                      "epoch_time": "Heure",
                      "puissance": "Puissance [W]"
                  },
                  title="Puissance mesurée pendant la journée")

    # work around for recent measurement
    query = "select * from puissance_table ORDER by epoch_time DESC limit 1"
    with server.app_context():
        result = get_db().cursor().execute(query).fetchone()  # (epoch, puissance)
    date = dt.fromtimestamp(result[0], pytz.timezone('Europe/Paris'))
    return fig, "{:.0f} W".format(result[1]), date.strftime("%d/%m/%Y - %H:%M")

@app.callback(
    [Output('daily-energy-plot', 'figure'),
     Output('monthly-energy-text', 'children'),
     Output('monthly-title', 'children')],
    [Input('daily-energy-month-picker', 'date')])
def update_daily_energy_plot(date):
    #date is initially in str type
    # prepare date info
    date = dt.strptime(date, "%Y-%m-%d")
    nb_days = monthrange(date.year, date.month)[1]
    start_time = dt(date.year, date.month, 1, tzinfo=pytz.timezone('Europe/Paris'))
    end_time = dt(date.year, date.month, nb_days, 23, 59, 59, tzinfo=pytz.timezone('Europe/Paris'))
    start_epoch = int(dt.timestamp(start_time))
    end_epoch = int(dt.timestamp(end_time))
    # get data from the database
    query = "select * from puissance_table where epoch_time BETWEEN {} and {}".format(
        start_epoch, end_epoch)
    # df = pd.read_sql_query(query, conn)
    with server.app_context():
        df = pd.read_sql_query(query, get_db())
    df['month'] = pd.to_datetime(
        df.epoch_time, unit='s').apply(lambda x: x.month)
    df['day'] = pd.to_datetime(df.epoch_time, unit='s').apply(lambda x: x.day)
    df = df[df.month == date.month]
    # total energy in this month
    energy = integrate.trapz(y=df.puissance, x=df.epoch_time)  # in W x second
    energy = energy/1000.0/86400.0  # in kWh
    # calculate daily energy in this month
    dailyEnergy = df.groupby(df.day).apply(
        lambda g: integrate.trapz(y=g.puissance, x=g.epoch_time))  # in W x s
    dailyEnergy = dailyEnergy/1000.0/86400.0  # in kWh
    # create the output figure
    fig = px.bar(x=dailyEnergy.index, y=dailyEnergy.values,
                 labels={
                     "x": "Jour",
                     "y": "Energie [kWh]"
                 },
                 title="Energie consommée pendant {}/{}".format(date.month, date.year))
    return fig, "{:.2f} kWh".format(energy), "Energie totale {}/{}".format(date.month, date.year)


# ========== Web layout
app.layout = html.Div(className="grid-container", children=[
    html.Div([
        recent_measure(),
        hourly_measure(),
    ], className="power-dash"),
    html.Div([
        daily_energy(),
        monthly_energy()
    ], className="energy-dash")

])


# =========== main
if __name__ == '__main__':
    app.run_server(host="0.0.0.0", debug=True)
    #     cur.close()
    #     conn.close()
    #     # print("Bye")