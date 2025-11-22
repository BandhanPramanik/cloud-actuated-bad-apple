import discord
from discord import app_commands
from discord.ext import commands
import paho.mqtt.client as mqtt

# --- CONFIGURATION ---
DISCORD_TOKEN = "ENTER_YOUR_TOKEN_HERE"
GUILD_ID = discord.Object(id=136247354655435242345) # <--- PASTE YOUR SERVER ID HERE!
MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC = "TOPIC"

# --- MQTT SETUP ---
mqtt_client = mqtt.Client()
mqtt_client.connect(MQTT_BROKER, 1883)
mqtt_client.loop_start()

# --- BOT SETUP ---
# We need 'intents' to listen to basic events
intents = discord.Intents.default()
client = commands.Bot(command_prefix="!", intents=intents)

# --- THE SYNC PROCESS (The "Awareness") ---
@client.event
async def on_ready():
    # This copies the global commands over to your specific server
    # so they appear INSTANTLY.
    client.tree.copy_global_to(guild=GUILD_ID)
    await client.tree.sync(guild=GUILD_ID)
    
    print(f'Logged in as {client.user}')
    print('Slash commands synced to server!')

# --- THE COMMANDS ---

@client.tree.command(name="play", description="Start the Bad Apple video")
async def play(interaction: discord.Interaction):
    mqtt_client.publish(MQTT_TOPIC, "PLAY")
    # We must respond to the interaction within 3 seconds
    await interaction.response.send_message("▶️ Sent PLAY command to ESP32!")

@client.tree.command(name="pause", description="Freeze the video")
async def pause(interaction: discord.Interaction):
    mqtt_client.publish(MQTT_TOPIC, "PAUSE")
    await interaction.response.send_message("⏸️ Sent PAUSE command.")

@client.tree.command(name="resume", description="Resume playing")
async def resume(interaction: discord.Interaction):
    mqtt_client.publish(MQTT_TOPIC, "RESUME")
    await interaction.response.send_message("⏯️ Sent RESUME command.")

@client.tree.command(name="stop", description="Stop video and clear screen")
async def stop(interaction: discord.Interaction):
    mqtt_client.publish(MQTT_TOPIC, "STOP")
    await interaction.response.send_message("⏹️ Sent STOP command.")

# Run the bot
client.run(DISCORD_TOKEN)
